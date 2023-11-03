#pragma once

extern "C"{
    #include <SDL.h>
}

class AudioPlay
{
public:
    AudioPlay();
    int openDevice(const SDL_AudioSpec *spec);
    void start();
    void stop();

private:
    SDL_AudioDeviceID m_devId = -1;
};