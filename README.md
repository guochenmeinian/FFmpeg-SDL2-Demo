# FFmpeg & SDL2 视频处理
该项目是在[B站（哔哩哔哩）](https://www.bilibili.com/)实习时做的Demo，主要演示了如何在 C++ 中使用 FFmpeg 和 SDL2 处理视频文件。其功能包括：
- 获取视频信息
- 解复用视频和音频流
- 将视频解码为YUV格式
- 使用SDL2显示YUV视频
- 使用 SDL2 播放 PCM 音频


## 要求
- FFmpeg 库
- SDL2 库


## Demo 实现
### 1. 实现本地mp4/flv视频的信息读取
```
g++ -o get_info get_info.cpp -lavformat -lavcodec -lavutil -lswscale -lswresample -lavdevice -lsdl2
./get_info inputs/sample.mp4

或者

ffmpeg -i inputs/sample.mp4
```

### 2. 实现本地mp4/flv视频解复用，视频流保存到本地为264/265文件，音频流保存到本地为aac文件，并通过ffmpeg命令行进行准确播放

-   c++代码实现：

    ```
    g++ -o mp4_to_h264 mp4_to_h264.cpp -lavformat -lavcodec -lavutil
    g++ -o mp4_to_aac mp4_to_aac.cpp -lavformat -lavcodec -lavutil

    ./mp4_to_h264 inputs/sample.mp4 inputs/sample.h264
    ./mp4_to_aac inputs/sample.mp4 inputs/sample.aac
    ```

    或者 

-   FFmpeg命令行实现：
    ```
    ffmpeg -i inputs/sample.mp4 -c:v copy -an inputs/sample.h264
    ffmpeg -i inputs/sample.mp4 -c:v copy -an inputs/sample.hevc
    ffmpeg -i inputs/sample.mp4 -c:a copy -vn inputs/sample.aac
    ``` 

转换成功后，播放使用以下FFmpeg命令行：

```
ffplay -f h264 -i inputs/sample.h264
ffplay -i inputs/sample.aac
```

### 3. 实现本地mp4/flv视频解复用，解码
- 3.1 保存YUV数据到本地，用ffmpeg命令行播放
    
    - c++代码实现：

        ```
        g++ -o save_yuv save_yuv.cpp -lavformat -lavcodec -lavutil -lswscale

        ./save_yuv inputs/sample.mp4
        ```
    
    - FFmpeg命令行实现：
        ```
        ffmpeg -i inputs/sample.mp4 -an -c:v rawvideo -pix_fmt yuv420p inputs/sample.yuv
        ```
    
    转换成功后，播放使用以下FFmpeg命令行：
    ```
    ffplay -f rawvideo -pixel_format yuv420p -video_size 640x360 inputs/sample.yuv
    ```

- 3.2 使用SDL2进行视频显示，并根据视频的帧间隔进行同步
    ```
    g++ -o sdl_video sdl_video.cpp -lSDL2
    ./sdl_video inputs/sample.yuv
    ```

### 4. 实现本地mp4/flv视频解复用，解码
- 4.1 保存PCM数据到本地，用ffmpeg命令行播放

    C++代码实现 (需要先通过前面的步骤将mp4转换成aac)
    ```
    g++ -o save_pcm save_pcm.cpp -lavformat -lavcodec -lavutil -lswscale -lswresample
    ./save_pcm inputs/sample.aac inputs/sample.pcm 
    ```
    
    播放
    ```
    ffplay -ar 44100 -f s32le inputs/sample.pcm
    ```
- 4.2 调用SDL2进行音频的播放
    ```
    g++ -o sdl_audio sdl_audio.cpp -lSDL2
    ./sdl_audio inputs/sample.pcm
    ```

### 5. 实现本地mp4/flv视频的解复用，解码，同时利用SDL2进行视频与音频的播放 
```
g++ -std=c++11 -o sdl_full sdl_full.cpp -lSDL2
./sdl_full inputs/sample.pcm inputs/sample.yuv
```


### Note
可以用 `make`编译所有可执行文件 或者用 `make clean`来清理所有生成的可执行文件。
