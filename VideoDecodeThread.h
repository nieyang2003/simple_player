/**
 * @file VideoDecodeThread.h
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
struct AVFrame;

/// @brief 视频解码线程
class VideoDecodeThread : public ThreadBase
{
public:
    VideoDecodeThread();

    void setPlayerCtx(FFmpegPlayerCtx *playerCtx) {
        m_playerCtx = playerCtx;
    }

    void run();

private:
    int videoEntry();
    int queuePicture(FFmpegPlayerCtx *playerCtx, AVFrame *pFrame, double pts);

private:
    FFmpegPlayerCtx *m_playerCtx = nullptr;

};