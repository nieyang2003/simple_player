#include "Timer.h"

/**
 * @brief 向SDL事件循环添加用户事件
 * 
 * @param interval 
 * @param param 回调函数
 * @return Uint32 
 */
static Uint32 callbackfunc(Unit32 interval, void *param)
{
    SDL_Event event;
    SDL_UserEvent userevent;

    userevent.type = SDL_USEREVENT;
    userevent.code = 0;
    userevent.data1 = param;
    userevent.data2 = NULL;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent(&event);

    return interval;
}

Timer::Timer()
{
}

/**
 * @brief 将需要定时调用的回调函数添加到计时器中
 * 
 * @param cb 回调函数
 * @param interval 时间间隔
 */
void Timer::start(void *cb, int interval)
{
    if (m_timerId != 0) {
        return;
    }

    /// @fn SDL_AddTimer
    /// @brief 注册一个定时器回调函数，该回调函数将在指定的时间间隔内周期性地执行
    // 使用SDL定时器，每隔固定时间
    SDL_TimerId timerId = SDL_AddTimer(interval, callbackfunc, cb);
    if (timerId == 0) {
        return ;
    }

    m_timerId = timerId;
}

/**
 * @brief 停止计时器
 * 
 */
void Timer::stop()
{
    if (m_timerId != 0) {
        SDL_RemoveTimer(m_timerId);
        m_timerId = 0;
    }
}
