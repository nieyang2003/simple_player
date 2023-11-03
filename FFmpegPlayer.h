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

#include "PacketQueue.h"

#define MAX_AUDIO_FRAME_SIZE        (1024)
#define VIDEO_PICTURE_QUEUE_SIZE    (1024)
#define MAX_FILE_NAME_LENGTH        (1024)

typedef void (*Image_Cb)(unsigned char* data, int w, int h, void *userdata);

enum PauseState {
    UNPAUSE = 0,
    PAUSE   = 1
};

struct VideoPicture {
    AVFrame  *bmp = nullptr;
    double pts = 0.0;
};

struct FFmpegPlayerCtx {
    // ffmpeg播放参数
    AVFormatContext     *formatCtx = nullptr;   /**< 多媒体文件格式上下文 */
    AVCodecContext      *aCodecCtx = nullptr;   /**< 音频编解码器上下文 */
    ACCodecContext      *vCodecCtx = nullptr;   /**< 视频编解码器上下文 */ 

    int                 audioStream = -1;       /**< 音频流索引 */
    int                 videoStream = -1;       /**< 视频流索引 */ 

    AVStream            *audio_st = nullptr;    /**< 音频流 */
    AVStream            *video_st = nullptr;    /**< 视频流 */

    PacketQueue         audio_queue;            /**< 音频流数据包队列 */
    PacketQueue         video_queue;            /**< 视频流数据包队列 */

    // 音频
    uint8_t             audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];  /**< 音频数据缓存区 */ 
    unsigned int        audio_buf_size  = 0;
    unsigned int        audio_buf_index = 0;
    AVFrame             *audio_frame  = nullptr;
    AVPacket            *audio_packet = nullptr;
    uint8_t             *audio_packet_data = nullptr;
    int                 audio_packet_size  = 0;

    // 记录seek操作的上下文
    std::atomic<int>    seek_require;   /**< 跳转操作需求 */  
    int                 seek_flags;
    int64_t             seek_pos;

    // seek操作状态机
    std::atomic<bool>   flush_a_ctx = false;
    std::atomic<bool>   flush_v_ctx = false;

    // 音视频同步参数
    double              audio_clock = 0.0;
    double              video_clock = 0.0;
    double              frame_timer = 0.0;
    double              frame_last_pts = 0.0;   /**< 最后显示 */
    double              frame_last_delay = 0.0;

    // 图像队列
    VideoPicture        picture_queue[VIDEO_PICTURE_QUEUE_SIZE];
    std::atomic<int>    picture_size   = 0;
    std::atomic<int>    picture_read_index = 0;     // 下一要读取图像索引
    std::atomic<int>    picture_write_index = 0;    // 下一要写入图像索引
    SDL_mutex           *picture_mutex = nullptr;
    SDL_cond            *picture_cond  = nullptr;

    char                filename[MAX_FILE_NAME_LENGTH];

    SwsContext          *sws_ctx = nullptr; /**< 图像缩放、颜色空间转换结构体 */
    SwrContext          *swr_ctx = nullptr; /**< 采样率转换结构体 */

    std::atomic<int>    pause_state = PauseState::UNPAUSE;
    // 图像回调
    Image_Cb            imgCb   = nullptr;  /**< 回调函数，解码后获取图像数据 */
    void                *cbData = nullptr;

    /// @brief 初始化结构体
    void init()
    {
        audio_frame   = av_frame_alloc();
        audio_packet  = av_packet_alloc();

        picture_mutex = SDL_CreateMutex();
        picture_cond  = SDL_CreateCond();
    }

    /// @brief 清理结构体
    void fini()
    {
        if (audio_frame) {
            av_frame_free(audio_frame);
        }

        if (audio_packet) {
            av_packet_free(&audio_packet);
        }

        if (picture_mutex) {
            SDL_DestoryMutex(picture_mutex);
        }

        if (picture_cond) {
            SDL_DestoryCond(picture_cond);
        }
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