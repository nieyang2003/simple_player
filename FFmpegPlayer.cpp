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
    m_playerCtx.imgCb = cb;
    m_playerCtx.cbData = userData;
}

/**
 * @brief 初始化播放器
 * 
 * @return int 
 */
int FFmpegPlayer::initPlayer()
{
    m_playerCtx.init();
    strncpy(m_playerCtx->filename, m_filePath, m_filePath.size());

    // 解封装线程
    m_demuxThread = new DemuxThread;
    m_demuxThread->setPlayerCtx(m_playerCtx);
    if (m_demuxThread->initDemuxThread() != 0)
    {
        log_println("DemuxThread init Failed.");
        return -1;
    }

    // 音频解码线程
    m_audioDecodeThread = new AudioDecodeThread;
    m_audioDecodeThread->setPlayerCtx(m_playerCtx);

    // 视频解码线程
    m_videoDecodeThread = new VideoDecodeThread;
    m_videoDecodeThread->setPlayerCtx(m_playerCtx);

    // 播放设备参数
	audio_wanted_spec.freq = 48000;
	audio_wanted_spec.format = AUDIO_S16SYS;
	audio_wanted_spec.channels = 2;
	audio_wanted_spec.silence = 0;
	audio_wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
	audio_wanted_spec.callback = FN_Audio_Cb;
	audio_wanted_spec.userdata = m_audioDecodeThread;
    // 创建音频播放器
    m_audioPlay = new AudioPlay;
    if (m_audioPlay->openDevice(&audio_wanted_spec) <= 0)
    {
        log_println("open audio device Failed.");
        return -1;
    }
    
    // 
    auto refreshEvent = [this](SDL_Event* e) {
        onRefreshEvent(e);
    }

	auto keyEvent = [this](SDL_Event* e) {
		onKeyEvent(e);
	};

	sdlApp->registerEvent(FF_REFRESH_EVENT, refreshEvent);
	sdlApp->registerEvent(SDL_KEYDOWN, keyEvent);

    return 0;
}

/**
 * @brief 开始播放
 * 
 */
void FFmpegPlayer::start()
{
    m_demuxThread->start();
    m_videoDecodeThread->start();
    m_audioDecodeThread->start();
    m_audioPlay->start();

    schedule_refresh(&m_playerCtx, 40);

    m_stop = false;
}

#define FREE(x) \
    delete x; \
    x = nullptr

/**
 * @brief 退出播放
 * 
 */
void FFmpegPlayer::stop()
{
    m_stop = true;

    // 停止音频解码
    log_println("audio decode thread clean...");
    if (m_audioDecodeThread)
    {
        m_audioDecodeThread->stop();
        FREE(m_audioDecodeThread);
    }
    log_println("audio decode thread finished.");

    // 停止音频播放线程
    log_println("audio play thread clean...");
	if (m_audioPlay) {
		m_audioPlay->stop();
		FREE(m_audioPlay);
	}
    log_println("audio device finished.");

    // 停止视频解码线程
    log_println("video decode thread clean...");
	if (m_videoDecodeThread) {
		m_videoDecodeThread->stop();
		FREE(m_videoDecodeThread);
	}
    log_println("video decode thread finished.");

    // 停止解封装线程
    log_println("demux thread clean...");
	if (m_demuxThread) {
		m_demuxThread->stop();
		m_demuxThread->finiDemuxThread();
		FREE(m_demuxThread);
	}
    log_println("demux thread finished.");

    // 释放播放器上下文
    log_println("player ctx clean...");
    m_playerCtx.fini();
    log_println("player ctx finished.");
}

/**
 * @brief 暂停播放
 * 
 * @param state 
 */
void FFmpegPlayer::pause(PauseState state)
{
    m_playerCtx.pause_state = true;
    m_playerCtx.frame_timer = av_gettime() / 1000000.0;   // 获取当前时间转为秒
}

/**
 * @brief 刷新事件
 * 
 * @param event 
 */
void FFmpegPlayer::onRefreshEvent(SDL_Event *event)
{
    if (m_stop)
    {
        return;
    }

    FFmpegPlayerCtx* p_playerCtx = (FFmpegPlayerCtx*)event->user.data1;
    VideoPicture* vp;
    double actual_delay, delay, sync_threshold, ref_clock, diff;

    if (p_playerCtx->video_st)
    {
        if (p_playerCtx->picture_size == 0)
        {
            schedule_refresh(p_playerCtx, 1);   // 延时1ms
        }
        else
        {
            vp = p_playerCtx->picture_queue[p_playerCtx->picture_read_index];

            delay = vp->pts - p_playerCtx->frame_last_pts;

            if (delay <= 0 || delay >= 1.0)
            {
                delay = p_playerCtx->frame_last_delay;
            }

            p_playerCtx->frame_last_delay = delay;
            p_playerCtx->frame_last_pts = vp->pts;

            ref_clock = get_audio_clock(p_playerCtx);
            diff = vp->pts - ref_clock;

            sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
            if (fabs(diff) < AV_NOSYNC_THRESHOLD)
            {
                if (diff <= -sync_threshold)
                {
                    delay = 0;
                } else if (diff >= sync_threshold)
                {
                    delay *= 2;
                }
            }

            p_playerCtx->frame_timer += delay;
            actual_delay = p_playerCtx->frame_timer - (av_gettime() / 1000000.0);
            if (actual_delay < 0.010)
            {
                actual_delay = 0.010;
            }

			schedule_refresh(p_playerCtx, (int)(actual_delay * 1000 + 0.5));

			video_display(p_playerCtx);

			if (++p_playerCtx->picture_read_index == VIDEO_PICTURE_QUEUE_SIZE)
            {
                p_playerCtx->picture_read_index = 0;
			}
			SDL_LockMutex(p_playerCtx->picture_mutex);
            p_playerCtx->picture_size--;
			SDL_CondSignal(p_playerCtx->picture_cond);
			SDL_UnlockMutex(p_playerCtx->picture_mutex);
        }
    }
    else
    {
        schedule_refresh(p_playerCtx, 100);
    }
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
