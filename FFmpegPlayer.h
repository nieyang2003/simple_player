#pragma once

#ifdef __cplusplus
extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include <SDL.h>
}
#endif

#include <string>
#include <atomic>

typedef void (*Image_Cb)(unsigned char* data, int w, int h, void *userdata);

enum PauseState {
    UNPAUSE = 0,
    PAUSE   = 1
};

struct FFmpegPlayerCtx {
    // ffmpeg播放参数
        // 输入，视频解码器，音频解码器
        // 音视频流数量与指针
    // 记录seek操作的上下文
    // seek操作状态机
    // 音视频同步参数
    // 图像队列
    // 图像回调
    // 初始、结束方法
    void init()
    {

    }

    void fini()
    {

    }
};

class DemuxThread;
class VideoDecodeThread;
class AudioDecodeThread;
class AudioPlay;

class FFmpegPlayer
{
public:
    FFmpegPlayer();
    ~FFmpegPlayer() = default;

    void setFilePath(const char *filePath);

    void setImageCb(Image_Cb cb, void *userData);

    int initPlayer();

    void start();
    
    void stop();

    void pause(PauseState state);

public:
    void onRefreshEvent(SDL_Event *event);

    void onKeyEvent(SDL_Event *event);

private:
    FFmpegPlayerCtx playerCtx;
    std::string m_filePath;
    SDL_AudioSpec audio_wanted_spec;
    std::atomic<bool> m_stop = false;
    
private:
    DemuxThread *m_demuxThread = nullptr;
    VideoDecodeThread *m_videoDecodeThread = nullptr;
    AudioDecodeThread *m_audioDecodeThread = nullptr;
    AudioPlay *m_audioPlay = nullptr;
};