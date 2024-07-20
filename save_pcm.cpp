#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
    #include <libavutil/frame.h>
    #include <libavutil/mem.h>
    #include <libavcodec/avcodec.h>
}

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

// 错误处理缓冲区
static char err_buf[128] = {0};

// 获取错误信息
static char* av_get_err(int errnum)
{
    av_strerror(errnum, err_buf, 128);
    return err_buf;
}

// 打印音频帧的采样格式信息
static void print_sample_format(const AVFrame *frame)
{
    printf("采样率: %uHz\n", frame->sample_rate);
    printf("声道数: %u\n", frame->ch_layout.nb_channels);
    printf("格式: %u\n", frame->format); // 注意：实际存储到本地文件时已经改成交错模式
}

// 解码函数，将音频包解码成音频帧并写入输出文件
static void decode(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame, FILE *outfile)
{
    int i, ch;
    int ret, data_size;

    // 发送包给解码器
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret == AVERROR(EAGAIN))
    {
        fprintf(stderr, "Receive_frame 和 send_packet 都返回 EAGAIN，这是一个 API 违规。\n");
    }
    else if (ret < 0)
    {
        fprintf(stderr, "提交包给解码器时出错, err:%s, pkt_size:%d\n",
                av_get_err(ret), pkt->size);
        return;
    }

    // 从解码器中读取所有输出帧
    while (ret >= 0)
    {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0)
        {
            fprintf(stderr, "解码时出错\n");
            exit(1);
        }
        data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
        if (data_size < 0)
        {
            fprintf(stderr, "计算数据大小失败\n");
            exit(1);
        }
        static int s_print_format = 0;
        if (s_print_format == 0)
        {
            s_print_format = 1;
            print_sample_format(frame);
        }

        // 写入交错模式的音频数据到输出文件
        for (i = 0; i < frame->nb_samples; i++)
        {
            for (ch = 0; ch < dec_ctx->ch_layout.nb_channels; ch++)
                fwrite(frame->data[ch] + data_size * i, 1, data_size, outfile);
        }
    }
}

int main(int argc, char **argv)
{
    const char *outfilename;
    const char *filename;
    const AVCodec *codec;
    AVCodecContext *codec_ctx = NULL;
    AVCodecParserContext *parser = NULL;
    int len = 0;
    int ret = 0;
    FILE *infile = NULL;
    FILE *outfile = NULL;
    uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data = NULL;
    size_t data_size = 0;
    AVPacket *pkt = NULL;
    AVFrame *decoded_frame = NULL;

    // 检查命令行参数
    if (argc <= 2)
    {
        fprintf(stderr, "用法: %s <输入文件> <输出文件>\n", argv[0]);
        exit(0);
    }
    filename = argv[1];
    outfilename = argv[2];

    // 分配AVPacket
    pkt = av_packet_alloc();
    enum AVCodecID audio_codec_id = AV_CODEC_ID_AAC;
    if (strstr(filename, "aac") != NULL)
    {
        audio_codec_id = AV_CODEC_ID_AAC;
    }
    else if (strstr(filename, "mp3") != NULL)
    {
        audio_codec_id = AV_CODEC_ID_MP3;
    }
    else
    {
        printf("默认编解码器ID:%d\n", audio_codec_id);
    }

    // 查找解码器
    codec = avcodec_find_decoder(audio_codec_id);
    if (!codec)
    {
        fprintf(stderr, "未找到编解码器\n");
        exit(1);
    }
    // 初始化解析器
    parser = av_parser_init(audio_codec_id);
    if (!parser)
    {
        fprintf(stderr, "未找到解析器\n");
        exit(1);
    }
    // 分配编解码器上下文
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx)
    {
        fprintf(stderr, "无法分配音频编解码器上下文\n");
        exit(1);
    }

    // 打开编解码器
    if (avcodec_open2(codec_ctx, codec, NULL) < 0)
    {
        fprintf(stderr, "无法打开编解码器\n");
        exit(1);
    }

    // 打开输入文件
    infile = fopen(filename, "rb");
    if (!infile)
    {
        fprintf(stderr, "无法打开 %s\n", filename);
        exit(1);
    }
    // 打开输出文件
    outfile = fopen(outfilename, "wb");
    if (!outfile)
    {
        av_free(codec_ctx);
        exit(1);
    }

    // 读取文件数据
    data = inbuf;
    data_size = fread(inbuf, 1, AUDIO_INBUF_SIZE, infile);

    while (data_size > 0)
    {
        if (!decoded_frame)
        {
            if (!(decoded_frame = av_frame_alloc()))
            {
                fprintf(stderr, "无法分配音频帧\n");
                exit(1);
            }
        }

        ret = av_parser_parse2(parser, codec_ctx, &pkt->data, &pkt->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (ret < 0)
        {
            fprintf(stderr, "解析时出错\n");
            exit(1);
        }
        data += ret;
        data_size -= ret;

        if (pkt->size)
            decode(codec_ctx, pkt, decoded_frame, outfile);

        if (data_size < AUDIO_REFILL_THRESH)
        {
            memmove(inbuf, data, data_size);
            data = inbuf;
            len = fread(data + data_size, 1, AUDIO_INBUF_SIZE - data_size, infile);
            if (len > 0)
                data_size += len;
        }
    }

    // 冲刷解码器
    pkt->data = NULL;
    pkt->size = 0;
    decode(codec_ctx, pkt, decoded_frame, outfile);

    // 关闭文件
    fclose(outfile);
    fclose(infile);

    // 释放资源
    avcodec_free_context(&codec_ctx);
    av_parser_close(parser);
    av_frame_free(&decoded_frame);
    av_packet_free(&pkt);

    printf("解码完成，请按 Enter 退出\n");
    return 0;
}
