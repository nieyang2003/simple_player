#include "AudioPlay.h"

AudioPlay::AudioPlay()
{
}

int AudioPlay::openDevice(const SDL_AudioSpec *spec)
{
    /// @fn SDL_OpenAudioDevice
    /// @brief 打开音频设备
    /// @param device 字符串：表示设备名，为NULL使用默认
    /// @param iscapture 整数，非0打开音频捕获设备，0打开播放设备
    /// @param obtained 指向 SDL_AudioSpec 结构体的指针，用于存储实际打开的音频设备的参数
    /// @param allowed_changes 非0允许音频参数变化
    /// @return int 0打开失败，非负数标识打开设备ID
    m_devId = SDL_OpenAudioDevice(NULL, 0, spec, NULL, 0);
    return m_devId;
}

void AudioPlay::start()
{
    /**
     * @fn SDL_PauseAudioDevice
     * @brief 暂停或继续音频设备的播放或捕获
     * @param pause int，非0表示暂停
     */
    SDL_PauseAudioDevice(m_devId, 0);
}

void AudioPlay::stop()
{
    SDL_PauseAudioDevice(m_devId, 1);
}
