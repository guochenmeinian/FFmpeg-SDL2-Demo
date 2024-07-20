#include <stdio.h>

// 包含FFmpeg库的头文件，使用extern "C"以避免C++编译器对C库函数名进行修饰
extern "C" {
    #include "libavutil/log.h"
    #include "libavformat/avio.h"
    #include "libavformat/avformat.h"
}

#define AacHeader

/* 相关blog文档链接：https://www.cnblogs.com/vczf/p/13553149.html */

// 支持的AAC采样频率数组，用于ADTS头部的生成
const int sampling_frequencies[] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000
};

// 生成ADTS头部的函数，用于在写入AAC数据前添加ADTS头部
int adts_header(char* const p_adts_header, const int data_length,
                const int profile, const int samplerate,
                const int channels) {

    // 默认采样频率索引为48000Hz
    int sampling_frequency_index = 3; 
    int adtsLen = data_length + 7; // ADTS头部长度为7字节

    // 确定采样频率在数组中的索引
    int frequencies_size = sizeof(sampling_frequencies) / sizeof(sampling_frequencies[0]);
    int i = 0;
    for (i = 0; i < frequencies_size; i++) {
        if (sampling_frequencies[i] == samplerate) {
            sampling_frequency_index = i;
            break;
        }
    }
    if (i >= frequencies_size) {
        printf("Unsupported samplerate: %d\n", samplerate);
        return -1;
    }

    // 填充ADTS头部信息
    p_adts_header[0] = 0xff; // 同步字1
    p_adts_header[1] = 0xf1; // 同步字2, MPEG-4, Layer, protection absent
    p_adts_header[2] = (profile << 6) | (sampling_frequency_index << 2) | ((channels & 0x04) >> 2);
    p_adts_header[3] = ((channels & 0x03) << 6) | (adtsLen >> 11);
    p_adts_header[4] = (adtsLen & 0x7f8) >> 3;
    p_adts_header[5] = ((adtsLen & 0x07) << 5) | 0x1f;
    p_adts_header[6] = 0xfc;

    return 0;
}

// 提取AAC音频流的函数，输入和输出文件名作为参数
int extract_aac(const char* input_filename, const char* output_filename) {
    // 1. 打开输入文件，获取AVFormatContext
    AVFormatContext* ctx = NULL;
    int ret = avformat_open_input(&ctx, input_filename, NULL, NULL);
    if (ret < 0) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        printf("avformat_open_input %s failed: %s\n", input_filename, buf);
        return -1;
    }

    // 2. 获取流信息
    ret = avformat_find_stream_info(ctx, NULL);
    if (ret < 0) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        printf("avformat_find_stream_info %s failed: %s\n", input_filename, buf);
        avformat_close_input(&ctx);
        return -1;
    }

    // 3. 找到最佳音频流
    int audioIndex = av_find_best_stream(ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (audioIndex < 0) {
        printf("av_find_best_stream failed: %s\n", av_get_media_type_string(AVMEDIA_TYPE_AUDIO));
        avformat_close_input(&ctx);
        return -1;
    }

    // 4. 检查音频流是否为AAC编码
    if (ctx->streams[audioIndex]->codecpar->codec_id != AV_CODEC_ID_AAC) {
        printf("audio codec %d is not AAC.\n", ctx->streams[audioIndex]->codecpar->codec_id);
        avformat_close_input(&ctx);
        return -1;
    }

    // 5. 打开输出AAC文件
    FILE* fd = fopen(output_filename, "wb");
    if (fd == NULL) {
        printf("fopen open %s failed.\n", output_filename);
        avformat_close_input(&ctx);
        return -1;
    }

    // 6. 提取AAC数据
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        printf("Failed to allocate AVPacket.\n");
        fclose(fd);
        avformat_close_input(&ctx);
        return -1;
    }

    int len = 0;
    while (av_read_frame(ctx, pkt) >= 0) {
        if (pkt->stream_index == audioIndex) {
#ifdef AacHeader  // 如果定义了AacHeader，则写入ADTS头部
            char adts_header_buf[7] = {0};
            adts_header(adts_header_buf, pkt->size,
                        ctx->streams[audioIndex]->codecpar->profile,
                        ctx->streams[audioIndex]->codecpar->sample_rate,
                        ctx->streams[audioIndex]->codecpar->ch_layout.nb_channels); // 注：channels(int channe数量)被更新成了ch_layout.nb_channels
            fwrite(adts_header_buf, 1, 7, fd);  // 写入ADTS头部
#endif
            len = fwrite(pkt->data, 1, pkt->size, fd);  // 写入AAC数据
            if (len != pkt->size) {
                printf("Warning, length of written data isn't equal to pkt.size(%d, %d)\n", len, pkt->size);
            }
        }
        av_packet_unref(pkt);  // 释放包数据
    }

    av_packet_free(&pkt);
    fclose(fd);
    avformat_close_input(&ctx);

    return 0;
}

// 主函数，处理命令行参数并调用提取函数
int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s <input file> <output file>\n", argv[0]);
        return -1;
    }

    const char* input_filename = argv[1];
    const char* output_filename = argv[2];

    return extract_aac(input_filename, output_filename);
}
