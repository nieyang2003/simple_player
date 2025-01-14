/**
 * @file AudioDecodeThread.h
 * @author nieyang (nieyang2003@qq.com)
 * @brief 
 * @version 0.1
 * @date 2023-11-06
 * 
 * 
 */
#pragma once

#include "ThreadBase.h"

struct FFmpegPlayerCtx;

class AudioDecodeThread : public ThreadBase
{
public:
    AudioDecodeThread() {};

    void setPlayerCtx(FFmpegPlayerCtx *playerCtx) 
    {
        m_playerCtx = playerCtx;
    }

    void getAudioData(unsigned char *stream, int length);

    void run();

private:
    int audio_decode_frame(FFmpegPlayerCtx *playerCtx, double *ppts);

private:
    FFmpegPlayerCtx *m_playerCtx = nullptr;

};