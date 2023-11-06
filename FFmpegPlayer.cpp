#include "FFmpegPlayer.h"

#include "DemuxThread.h"
#include "VideoDecodeThread.h"
#include "AudioDecodeThread.h"
#include "AudioPlay.h"
#include "log.h"
#include "SDLApp.h"

#include <functional>

/**
 * @brief 计算音频时钟
 * 
 * @param player_ctx 播放器上下文
 * @return 返回音频时钟的值 
 */
static double get_audio_clock(FFmpegPlayerCtx *player_ctx)
{
    double pts; // 音频时钟值
    int hw_buf_size, bytes_per_sec, n;

    pts = player_ctx->audio_clock;  // 初始化音频时钟为当前值
    hw_buf_size = player_ctx->audio_buf_size - player_ctx->audio_buf_index; // 计算剩余缓存区大小
    bytes_per_sec = 0;
    n = player_ctx->aCodecCtx->ch_layout.nb_channels * 2;   // 通道布局通道数*2，计算样本数据字节数

    if (player_ctx->audio_st) {
        bytes_per_sec = player_ctx->aCodecCtx->sample_rate * n; // 每秒字节数
    }

    if (bytes_per_sec) {
        pts -= (double)(hw_buf_size / bytes_per_sec);   // 调整时钟值
    }

    return pts;
}

/**
 * @brief SDL定时器回调刷新函数
 * 
 * @param interval 定时器触发间隔(未使用)
 * @param opaque 不透明数据指针
 * @return Uint32 如果返回0，表示定时器的周期性触发将被取消
 */
static Uint32 sdl_refresh_timer_cb(Uint32 interval, void *opaque)
{
    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    event.user.data1 = opaque;
    SDL_PushEvent(&event);  // 将事件推送到SDL事件队列中

    return 0;
}

/**
 * @brief 添加播放器刷新事件定时器
 *
 * @param is FFmpegPlayerCtx 播放器上下文
 * @param delay 定时器触发的延迟时间（以毫秒为单位）
 */
static void schedule_refresh(FFmpegPlayerCtx *player_ctx, int delay)
{
    SDL_AddTimer(delay, sdl_refresh_timer_cb, player_ctx);
}

/**
 * @brief 显示视频
 * 
 * @param player_ctx 播放器上下文
 */
static void video_display(FFmpegPlayerCtx *player_ctx)
{   
    // 获取当前视频帧
    VideoPicture *video_pic = &(player_ctx->picture_queue[player_ctx->picture_read_index]);

    if (video_pic->bmp && player_ctx->imgCb) {
        // 如果存在视频帧和回调函数，则调用回调函数显示视频
        player_ctx->imgCb(video_pic->bmp->data[0], player_ctx->vCodecCtx->width, 
                          player_ctx->vCodecCtx->height, player_ctx->cbData);
    }
}

static void FN_Audio_Cb(void *userdata, Uint8 *stream, int len)
{
    AudioDecodeThread *a_decode_thread = (AudioDecodeThread*)userdata;
    a_decode_thread->(stream, len);
}

void stream_seek(FFmpegPlayerCtx *player_ctx, int64_t pos, int rel)
{
    if (!player_ctx->seek_require) {
        player_ctx->seek_pos = pos;
        player_ctx->seek_flags = rel < 0 ? AVSEEK_FLAG_BACKWARD: 0;
        player_ctx->seek_flags = 1; // true
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////

FFmpegPlayer::FFmpegPlayer()
{
}

void FFmpegPlayer::setFilePath(const char *filePath)
{
    m_filePath = filePath;
}

void FFmpegPlayer::setImageCb(Image_Cb cb, void *userData)
{

}

/**
 * @brief 初始化播放器
 * 
 * @return int 
 */
int FFmpegPlayer::initPlayer()
{
    return 0;
}

/**
 * @brief 开始
 * 
 */
void FFmpegPlayer::start()
{
}

/**
 * @brief 退出
 * 
 */
void FFmpegPlayer::stop()
{
}

/**
 * @brief 暂停
 * 
 * @param state 
 */
void FFmpegPlayer::pause(PauseState state)
{
}

/**
 * @brief 刷新事件
 * 
 * @param event 
 */
void FFmpegPlayer::onRefreshEvent(SDL_Event *event)
{
}

/**
 * @brief 按下按键事件
 * 
 * @param event 
 */
void FFmpegPlayer::onKeyEvent(SDL_Event *event)
{
    /// @todo 完成对应操作
    double incr, pos;
    switch (event->key.keysym.sym) {
        case SDLK_LEFT: {   // 左键快退
            ;
        } break;
        case SDLK_RIGHT: {  // 右键快进
            ;
        } break;
        case SDLK_UP: {     // 上键加音量
            ;
        } break;
        case SDLK_DOWN: {   // 下键减音量
            ;
        } break;
        case SDLK_ESC: {    // Esc退出
            ;
        } break;
        case SDLK_SPACE: {  // 空格暂停
            ;
        } break;

        default : break;    // 无效按键
    }
}
