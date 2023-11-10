#include "SDLApp.h"

#define SDL_APP_EVENT_TIMEOUT (1)

static SDLApp *globalInstance = nullptr;

SDLApp::SDLApp()
{
    SDL_Init(SDL_INIT_EVERYTHING);

    if (!globalInstance) {
        globalInstance = this;
    } else {
        fprintf(stderr, "only one instance allowed!");
        exit(1);
    }
}

/**
 * @brief 开始事件循环
 * 
 * @return int 
 */
int SDLApp::exec()
{
    SDL_Event event;
    while(true) {
        /// @fn SDL_WaitEventTimeout
        /// @brief 超时时间(ms)内等待事件发生，0立即返回，将事件存储在event中
        SDL_WaitEventTimeout(&event, SDL_APP_EVENT_TIMEOUT);

        switch (event.type)
        {
        case SDL_QUIT: {
            /// @fn SDL_Quit
            /// @brief 用于释放和关闭 SDL，清理 SDL 子系统的资源
            SDL_Quit();
            return 0;
        }
        case SDL_USEREVENT: {
            // 回调函数存在SDL_UserEvent.user.data1中
            // 注册的计时器发送用户事件在此被调用
            std::function<void()> cb = *(std::function<void()>*)event.user.data1;
            cb();
        } break;
        default:{
            auto iter = m_userEventMaps.find(event.type);
            if (iter != m_userEventMaps.end()){
                auto onEventCb = iter->second;
                onEventCb(&event);
            }
        } break;
        }
    }
    return 0;
}

void SDLApp::quit()
{
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

/**
 * @brief 向事件表中注册事件
 * 
 * @param type 事件类型
 * @param cb 事件触发回调函数
 */
void SDLApp::registerEvent(int type, const std::function<void(SDL_Event *)> &cb)
{
    m_userEventMaps[type] = cb;
}

SDLApp *SDLApp::instance()
{
    return globalInstance;
}
