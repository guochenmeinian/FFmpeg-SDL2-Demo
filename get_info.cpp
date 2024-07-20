#include <iostream>
#include <fstream>
#include <vector>
#include <string>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <SDL2/SDL.h>
}

/* 相关blog文档链接： https://www.cnblogs.com/vczf/p/13541559.html */

/* This function opens a media file, retrieves and prints its codec, resolution, bitrate, and audio properties. */
void get_media_info(const std::string &filename) {
    
    // AVFormatContext是描述一个媒体文件或媒体流的构成和基本信息的结构体
    AVFormatContext *formatContext = avformat_alloc_context();
    
    // 打开文件，主要是探测协议类型，如果是网络文件则创建网络链接
    if (avformat_open_input(&formatContext, filename.c_str(), NULL, NULL) != 0) {
        std::cerr << "Could not open file." << std::endl;
        return;
    }

    // 读取码流信息
    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        std::cerr << "Could not get stream info." << std::endl;
        avformat_close_input(&formatContext);
        return;
    }

    av_dump_format(formatContext, 0, filename.c_str(), 0);

    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        AVStream *stream = formatContext->streams[i];
        AVCodecParameters *codecPar = stream->codecpar;

        if (codecPar->codec_type == AVMEDIA_TYPE_VIDEO) {
            std::cout << "Video Codec: " << avcodec_get_name(codecPar->codec_id) << std::endl;
            std::cout << "Resolution: " << codecPar->width << "x" << codecPar->height << std::endl;
            std::cout << "Bitrate: " << codecPar->bit_rate << std::endl;
        } else if (codecPar->codec_type == AVMEDIA_TYPE_AUDIO) {
            std::cout << "Audio Codec: " << avcodec_get_name(codecPar->codec_id) << std::endl;
            std::cout << "Channels: " << codecPar->ch_layout.nb_channels << std::endl;
            std::cout << "Sample Rate: " << codecPar->sample_rate << std::endl;
        }
    }

    avformat_close_input(&formatContext);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <media_file>" << std::endl;
        return -1;
    }

    std::string filename = argv[1];
    get_media_info(filename);

    return 0;
}
