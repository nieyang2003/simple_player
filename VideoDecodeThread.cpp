#include "VideoDecodeThread.h"

#include "FFmpegPlayer.h"
#include "log.h"

static double synchronize_video(FFmpegPlayerCtx* playerCtx, AVFrame* src_frame, double pts)
{
    double frame_delay;

    if (pts != 0)
    {
        playerCtx->video_clock = pts;
    }
    else
    {
        pts = playerCtx->video_clock;
    }
    frame_delay = av_q2d(playerCtx->vCodecCtx->time_base);
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    playerCtx->video_clock += frame_delay;

    return pts;
}

VideoDecodeThread::VideoDecodeThread()
{
}

void VideoDecodeThread::run()
{
}

int VideoDecodeThread::videoEntry()
{
    AVPacket* pPacket = av_packet_alloc();
    AVCodecContext* pCodecCtx = m_playerCtx->vCodecCtx;
    int ret = -1;
    double pts = 0;

    AVFrame* pFrame = av_frame_alloc();
    AVFrame* pFrameRGB = av_frame_alloc();

    /// @fn av_image_alloc
    /// @brief ·ÖÅäÍ¼ÏñÖ¡»º³åÇø
    av_image_alloc(pFrameRGB->data, pFrameRGB->linesize, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, 32);

    while (true)
    {
        if (m_stop)
        {
            break;
        }

        if (m_playerCtx->pause_state = PAUSE)
        {
            SDL_Delay(5);
            continue;
        }

        if (m_playerCtx->flush_v_ctx)
        {
            log_println("avcodec_flush_buffers(vCodecCtx) for seeking");
            avcodec_flush_buffers(m_playerCtx->vCodecCtx);
            m_playerCtx->flush_v_ctx = true;
            continue;
        }

        av_packet_unref(pPacket);

        if (m_playerCtx->video_queue.packetGet(pPacket, m_stop) < 0)
        {
            break;
        }

        // ½âÂë
        ret = avcodec_send_packet(pCodecCtx, pPacket);
        if (ret == 0)
        {
            ret = avcodec_send_frame(pCodecCtx, pFrame);
        }
        // 
        if (pPacket->dts == AV_NOPTS_VALUE && pFrame->opaque && *(uint64_t*)pFrame->opaque != AV_NOPTS_VALUE)
        {
            pts = (double)*(uint64_t*)pFrame->opaque;
        } 
        else if (pPacket->dts != AV_NOPTS_VALUE)
        {
            pts = (double)pPacket->dts;
        }
        else
        {
            pts = 0;
        }
        pts *= av_q2d(m_playerCtx->video_st->time_base);

        // frame ready
		if (ret == 0) 
        {
			ret = sws_scale(m_playerCtx->sws_ctx, (uint8_t const* const*)pFrame->data, pFrame->linesize, 0,
				pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

			pts = synchronize_video(m_playerCtx, pFrame, pts);

			if (ret == pCodecCtx->height) {
				if (queuePicture(m_playerCtx, pFrameRGB, pts) < 0) {
					break;
				}
			}
		}
    }

	av_frame_free(&pFrame);
	av_frame_free(&pFrameRGB);
	av_packet_free(&pPacket);

	return 0;
}

int VideoDecodeThread::queuePicture(FFmpegPlayerCtx *playerCtx, AVFrame *pFrame, double pts)
{
    VideoPicture* vp;

    SDL_LockMutex(playerCtx->picture_mutex);
    while (playerCtx->picture_size >= VIDEO_PICTURE_QUEUE_SIZE)
    {
        SDL_CondWaitTimeout(playerCtx->picture_cond, playerCtx->picture_mutex, 500);
        if (m_stop)
        {
            break;
        }
    }
    SDL_UnlockMutex(playerCtx->picture_mutex);

    if (m_stop)
    {
        return 0;
    }

    // 
    vp = &playerCtx->picture_queue[playerCtx->picture_write_index];


    if (!vp->bmp)
    {
        SDL_LockMutex(playerCtx->picture_mutex);
        vp->bmp = av_frame_alloc();
        av_image_alloc(vp->bmp->data, vp->bmp->linesize, playerCtx->vCodecCtx->width, playerCtx->vCodecCtx->height, AV_PIX_FMT_RGB24, 32);
        SDL_UnlockMutex(playerCtx->picture_mutex);
    }

    //
	memcpy(vp->bmp->data[0], pFrame->data[0], playerCtx->vCodecCtx->height * pFrame->linesize[0]);
	vp->pts = pts;

	if (++playerCtx->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE) {
        playerCtx->pictq_windex = 0;
	}
	SDL_LockMutex(playerCtx->pictq_mutex);
    playerCtx->pictq_size++;
	SDL_UnlockMutex(playerCtx->pictq_mutex);

    return 0;
}
