#include <SDL2/SDL.h>
#include <iostream>
#include <fstream>
#include <thread>

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2
#define SAMPLE_FORMAT AUDIO_S16SYS
#define BUFFER_SIZE 4096

const int screen_width = 640;  // 修改为适合您的YUV文件的宽度
const int screen_height = 360; // 修改为适合您的YUV文件的高度

// 音频回调函数，将音频数据从文件读取到音频缓冲区中
void audio_callback(void* userdata, Uint8* stream, int len) {
    std::ifstream* audioFile = static_cast<std::ifstream*>(userdata);
    if (!audioFile->read(reinterpret_cast<char*>(stream), len)) {
        // 如果文件读取的数据少于 len，用静音数据填充剩余部分
        std::fill(stream + audioFile->gcount(), stream + len, 0);
    }
}

// 播放音频的函数
void play_audio(const char* audio_filename) {
    std::ifstream audioFile(audio_filename, std::ios::binary);
    if (!audioFile.is_open()) {
        std::cerr << "无法打开音频文件: " << audio_filename << "\n";
        return;
    }

    // 初始化SDL音频子系统
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "无法初始化SDL音频 - " << SDL_GetError() << "\n";
        return;
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
        return;
    }

    SDL_PauseAudio(0); // 开始播放音频

    // 等待音频播放完成
    while (!audioFile.eof()) {
        SDL_Delay(100);
    }

    SDL_CloseAudio(); // 关闭音频设备
    SDL_Quit(); // 清理所有初始化的SDL子系统
    audioFile.close(); // 关闭音频文件
}

// 播放视频的函数
void play_video(const char* video_filename) {
    std::ifstream yuvFile(video_filename, std::ios::binary);
    if (!yuvFile.is_open()) {
        std::cerr << "无法打开视频文件: " << video_filename << "\n";
        return;
    }

    // 初始化SDL视频子系统
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "无法初始化SDL视频 - " << SDL_GetError() << "\n";
        return;
    }

    // 创建SDL窗口
    SDL_Window* window = SDL_CreateWindow("YUV Player",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          screen_width, screen_height,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL: 无法创建窗口 - 退出: " << SDL_GetError() << "\n";
        return;
    }

    // 创建SDL渲染器
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        std::cerr << "SDL: 无法创建渲染器 - 退出: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    // 创建SDL纹理
    SDL_Texture* texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_IYUV,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             screen_width, screen_height);
    if (!texture) {
        std::cerr << "SDL: 无法创建纹理 - 退出: " << SDL_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    // 计算YUV数据的大小
    const int y_size = screen_width * screen_height;
    const int uv_size = y_size / 4;
    char* buffer = new char[y_size + 2 * uv_size]; // 分配YUV数据缓冲区

    bool quit = false;
    SDL_Event event;

    while (!quit) {
        // 处理SDL事件
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        // 读取YUV数据，如果读取失败则重新读取
        if (!yuvFile.read(buffer, y_size + 2 * uv_size)) {
            yuvFile.clear();
            yuvFile.seekg(0, std::ios::beg);
            yuvFile.read(buffer, y_size + 2 * uv_size);
        }

        // 更新纹理并渲染
        SDL_UpdateTexture(texture, nullptr, buffer, screen_width);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        SDL_Delay(40); // 大约每秒25帧
    }

    // 释放资源
    delete[] buffer;
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    yuvFile.close();
}

// 主函数，处理命令行参数并启动音频和视频播放
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "用法: " << argv[0] << " <输入PCM文件> <输入YUV文件>\n";
        return -1;
    }

    const char* audio_filename = argv[1];
    const char* video_filename = argv[2];

    // 在单独的线程中启动音频播放
    std::thread audio_thread([audio_filename]() { play_audio(audio_filename); });

    // 在主线程中启动视频播放
    play_video(video_filename);

    // 等待音频线程完成
    audio_thread.join();

    return 0;
}
