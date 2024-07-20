# Makefile

# 编译器和编译选项
CXX = g++
CXXFLAGS = -std=c++11 -lavformat -lavcodec -lavutil -lswscale -lswresample -lavdevice -lSDL2

# 可执行文件
EXECUTABLES = get_info mp4_to_h264 mp4_to_aac save_yuv save_pcm sdl_audio sdl_video sdl_full

# 默认目标：编译所有可执行文件
all: $(EXECUTABLES)

# 各个可执行文件的编译规则
get_info: get_info.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

mp4_to_h264: mp4_to_h264.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

mp4_to_aac: mp4_to_aac.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

save_yuv: save_yuv.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

save_pcm: save_pcm.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

sdl_audio: sdl_audio.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

sdl_video: sdl_video.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

sdl_full: sdl_full.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

# 清理编译生成的文件
clean:
	rm -f $(EXECUTABLES)
