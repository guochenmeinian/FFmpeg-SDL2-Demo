extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

#include <iostream>

// 保存视频流到输出文件的函数
void save_video_stream(const char* output_filename, AVFormatContext* input_format_context, AVStream* input_stream) {
    AVFormatContext* output_format_context = nullptr;
    AVStream* output_stream = nullptr;
    AVPacket packet;

    // 分配输出格式上下文
    avformat_alloc_output_context2(&output_format_context, nullptr, nullptr, output_filename);
    if (!output_format_context) {
        std::cerr << "无法创建输出上下文\n";
        return;
    }

    // 创建输出流
    output_stream = avformat_new_stream(output_format_context, nullptr);
    if (!output_stream) {
        std::cerr << "无法分配输出流\n";
        return;
    }

    // 复制输入流的编解码参数到输出流
    avcodec_parameters_copy(output_stream->codecpar, input_stream->codecpar);
    output_stream->codecpar->codec_tag = 0;

    // 打开输出文件
    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_format_context->pb, output_filename, AVIO_FLAG_WRITE) < 0) {
            std::cerr << "无法打开输出文件\n";
            return;
        }
    }

    // 写入文件头
    if (avformat_write_header(output_format_context, nullptr) < 0) {
        std::cerr << "打开输出文件时发生错误\n";
        return;
    }

    // 从输入文件读取帧并写入输出文件
    while (av_read_frame(input_format_context, &packet) >= 0) {
        if (packet.stream_index == input_stream->index) {
            // 重新调整时间戳
            packet.pts = av_rescale_q(packet.pts, input_stream->time_base, output_stream->time_base);
            packet.dts = av_rescale_q(packet.dts, input_stream->time_base, output_stream->time_base);
            packet.duration = av_rescale_q(packet.duration, input_stream->time_base, output_stream->time_base);
            packet.pos = -1;
            packet.stream_index = output_stream->index;

            // 写入数据包
            if (av_interleaved_write_frame(output_format_context, &packet) < 0) {
                std::cerr << "复用数据包时出错\n";
                break;
            }
        }
        av_packet_unref(&packet); // 释放数据包引用，减少内存使用
    }

    // 写入文件尾
    av_write_trailer(output_format_context);

    // 关闭输出文件
    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&output_format_context->pb);
    }

    // 释放输出格式上下文
    avformat_free_context(output_format_context);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <输入文件> <输出视频文件>\n";
        return -1;
    }

    const char* input_filename = argv[1];
    const char* output_video_filename = argv[2];

    AVFormatContext* input_format_context = nullptr;
    AVStream* video_stream = nullptr;

    // 打开输入文件并读取文件头
    if (avformat_open_input(&input_format_context, input_filename, nullptr, nullptr) < 0) {
        std::cerr << "无法打开输入文件\n";
        return -1;
    }

    // 获取流信息
    if (avformat_find_stream_info(input_format_context, nullptr) < 0) {
        std::cerr << "获取输入流信息失败\n";
        return -1;
    }

    // 查找视频流
    for (unsigned int i = 0; i < input_format_context->nb_streams; ++i) {
        AVStream* stream = input_format_context->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && !video_stream) {
            video_stream = stream;
        }
    }

    if (!video_stream) {
        std::cerr << "输入文件中未找到视频流\n";
        return -1;
    }

    // 保存视频流到输出文件
    save_video_stream(output_video_filename, input_format_context, video_stream);

    // 关闭输入文件
    avformat_close_input(&input_format_context);

    return 0;
}
