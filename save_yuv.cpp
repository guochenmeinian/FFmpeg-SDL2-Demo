extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
}

#include <iostream>
#include <fstream>
#include <string>

// 获取文件路径的父目录
std::string getParentDirectory(const std::string &filePath) {
    size_t pos = filePath.find_last_of("/\\");
    return (pos == std::string::npos) ? "." : filePath.substr(0, pos);
}

// 保存YUV帧数据到文件
void SaveFrame(AVFrame *pFrame, int width, int height, FILE *pFile) {
    int y;

    // 写入Y平面数据
    for (y = 0; y < height; y++)
        fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width, pFile);
    // 写入U平面数据
    for (y = 0; y < height / 2; y++)
        fwrite(pFrame->data[1] + y * pFrame->linesize[1], 1, width / 2, pFile);
    // 写入V平面数据
    for (y = 0; y < height / 2; y++)
        fwrite(pFrame->data[2] + y * pFrame->linesize[2], 1, width / 2, pFile);
}

// 处理MP4文件并保存为YUV格式
void ProcessMP4ToYUV(const std::string &inputFile) {
    AVFormatContext *pFormatCtx = nullptr;
    int videoStream;
    AVCodecContext *pCodecCtx = nullptr;
    const AVCodec *pCodec = nullptr;
    AVFrame *pFrame = nullptr;
    AVPacket packet;

    // 获取输出目录和输出文件路径
    std::string outputDir = getParentDirectory(inputFile);
    std::string outputFilePath = outputDir + "/sample.yuv";
    FILE *pFile = fopen(outputFilePath.c_str(), "wb");
    if (pFile == nullptr) {
        std::cerr << "无法打开输出文件: " << outputFilePath << std::endl;
        return;
    }

    // 打开输入文件
    if (avformat_open_input(&pFormatCtx, inputFile.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "无法打开输入文件: " << inputFile << std::endl;
        fclose(pFile);
        return;
    }

    // 获取流信息
    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
        std::cerr << "无法找到流信息" << std::endl;
        fclose(pFile);
        avformat_close_input(&pFormatCtx);
        return;
    }

    // 输出文件格式信息
    av_dump_format(pFormatCtx, 0, inputFile.c_str(), 0);

    // 查找视频流
    videoStream = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    if (videoStream == -1) {
        std::cerr << "未找到视频流" << std::endl;
        fclose(pFile);
        avformat_close_input(&pFormatCtx);
        return;
    }

    // 查找解码器
    pCodec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
    if (pCodec == nullptr) {
        std::cerr << "不支持的编解码器!" << std::endl;
        fclose(pFile);
        avformat_close_input(&pFormatCtx);
        return;
    }

    // 分配编解码器上下文
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar) < 0) {
        std::cerr << "无法复制编解码器上下文" << std::endl;
        fclose(pFile);
        avcodec_free_context(&pCodecCtx);
        avformat_close_input(&pFormatCtx);
        return;
    }

    // 打开编解码器
    if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
        std::cerr << "无法打开编解码器" << std::endl;
        fclose(pFile);
        avcodec_free_context(&pCodecCtx);
        avformat_close_input(&pFormatCtx);
        return;
    }

    // 分配AVFrame结构体
    pFrame = av_frame_alloc();
    if (pFrame == nullptr) {
        std::cerr << "无法分配AVFrame" << std::endl;
        fclose(pFile);
        avcodec_free_context(&pCodecCtx);
        avformat_close_input(&pFormatCtx);
        return;
    }

    // 读取帧数据并解码
    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        if (packet.stream_index == videoStream) {
            if (avcodec_send_packet(pCodecCtx, &packet) != 0) {
                av_packet_unref(&packet);
                continue;
            }
            while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                SaveFrame(pFrame, pCodecCtx->width, pCodecCtx->height, pFile);
            }
        }
        av_packet_unref(&packet);
    }

    // 释放资源
    fclose(pFile);
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);
}

// 主函数，处理命令行参数并调用处理函数
int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <输入文件>" << std::endl;
        return -1;
    }

    std::string inputFile = argv[1];
    ProcessMP4ToYUV(inputFile);

    return 0;
}
