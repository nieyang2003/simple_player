/**
 * @file DemuxThread.h
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

/**
 * @brief 多媒体文件解封装线程
 * 
 */
class DemuxThread : ThreadBase
{
public:
    DemuxThread();

    void setPlayerCtx(FFmpegPlayerCtx *playerCtx) {
        m_playerCtx = playerCtx;
    }

    int initDemuxThread();

    void finiDemuxThread();

    void run();

private:
    int decodeLoop();
    // int audioDecodeFrame();
    int streamOpen(FFmpegPlayerCtx* playerCtx, int mediaType);

private:
    FFmpegPlayerCtx *m_playerCtx = nullptr;
};