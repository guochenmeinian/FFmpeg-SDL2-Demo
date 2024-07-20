#include <SDL2/SDL.h>
#include <iostream>
#include <fstream>

const int screen_width = 640; // 修改为适合您的YUV文件的宽度
const int screen_height = 360; // 修改为适合您的YUV文件的高度

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <输入YUV文件>\n";
        return -1;
    }

    const char* input_filename = argv[1];
    // 打开输入的YUV文件
    std::ifstream yuvFile(input_filename, std::ios::binary);
    if (!yuvFile.is_open()) {
        std::cerr << "无法打开文件: " << input_filename << "\n";
        return -1;
    }

    // 初始化SDL视频子系统
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "无法初始化SDL - " << SDL_GetError() << "\n";
        return -1;
    }

    // 创建SDL窗口
    SDL_Window* window = SDL_CreateWindow("YUV Player",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          screen_width, screen_height,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL: 无法创建窗口 - 退出: " << SDL_GetError() << "\n";
        return -1;
    }

    // 创建SDL渲染器
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        std::cerr << "SDL: 无法创建渲染器 - 退出: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
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
        return -1;
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

    return 0;
}
