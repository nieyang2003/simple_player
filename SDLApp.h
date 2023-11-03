#pragma once

#include <map>
#include <functional>

#ifdef _cplusplus
extern "C" {
    #include <SDL.h>
}
#endif

#define sdlApp (SDLApp::instance())

class SDLApp
{
public:
    SDLApp();

public:
    int exec();
    void quit();
    void registerEvent(int type, const std::function<void(SDL_Event*)> &cb);
    static SDLApp* instance();

private:
    std::map<int, std::function<void(SDL_Event*)>> m_userEventMaps;

};