#include <SDL2/SDL.h>
#include <iostream>
#include <fstream>

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2
#define SAMPLE_FORMAT AUDIO_S16SYS
#define BUFFER_SIZE 4096

// 音频回调函数，将音频数据从文件读取到音频缓冲区中
void audio_callback(void* userdata, Uint8* stream, int len) {
    std::ifstream* audioFile = static_cast<std::ifstream*>(userdata);
    if (!audioFile->read(reinterpret_cast<char*>(stream), len)) {
        // 如果文件读取的数据少于 len，用静音数据填充剩余部分
        std::fill(stream + audioFile->gcount(), stream + len, 0);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <输入PCM文件>\n";
        return -1;
    }

    const char* input_filename = argv[1];
    // 打开输入的PCM文件
    std::ifstream audioFile(input_filename, std::ios::binary);
    if (!audioFile.is_open()) {
        std::cerr << "无法打开文件: " << input_filename << "\n";
        return -1;
    }

    // 初始化SDL音频子系统
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "无法初始化SDL - " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_AudioSpec wanted_spec;
    SDL_zero(wanted_spec); // 将wanted_spec结构体初始化为0
    wanted_spec.freq = SAMPLE_RATE; // 采样率
    wanted_spec.format = SAMPLE_FORMAT; // 采样格式
    wanted_spec.channels = NUM_CHANNELS; // 声道数
    wanted_spec.silence = 0; // 静音值
    wanted_spec.samples = BUFFER_SIZE; // 音频缓冲区大小
    wanted_spec.callback = audio_callback; // 回调函数
    wanted_spec.userdata = &audioFile; // 传递给回调函数的用户数据

    // 打开音频设备并开始播放
    if (SDL_OpenAudio(&wanted_spec, NULL) < 0) {
        std::cerr << "SDL_OpenAudio错误: " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_PauseAudio(0); // 开始播放音频

    std::cout << "正在播放音频，请按 Enter 退出...\n";
    std::cin.get(); // 等待用户按下Enter键

    SDL_CloseAudio(); // 关闭音频设备
    SDL_Quit(); // 清理所有初始化的SDL子系统
    audioFile.close(); // 关闭音频文件

    return 0;
}
