#include "FFmpegPlayer.h"

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
