#include "DemuxThread.h"
#include "FFmpegPlayer.h"
#include "log.h"

DemuxThread::DemuxThread()
{

}

int DemuxThread::initDemuxThread()
{
    AVFormatContext *formatCtx = NULL;
    if (avformat_open_input(&formatCtx, m_playerCtx->filename, NULL, NULL) != 0) {
        log_println("Failed to open input file");
        return -1;
    }

    m_playerCtx->formatCtx = formatCtx;

    if (avformat_find_stream_info(formatCtx, NULL) < 0) {
        log_println("avformat_find_stream_info failed");
        return -1;
    }

    av_dump_file(formatCtx, 0, m_playerCtx->filename, 0);

    if (streamOpen(m_playerCtx, AVMEDIA_TYPE_AUDIO) < 0) {
        log_println("Failed to open audio stream");
        return -1;
    }

    if (streamOpen(m_playerCtx, AVMEDIA_TYPE_VIDEO) < 0) {
        log_println("Failed to open video stream");
        return -1;
    }

    return 0;
}

void DemuxThread::finiDemuxThread()
{
    if (is->formatCtx) {
        avformat_close_input(&is->formatCtx);
        is->formatCtx = nullptr;
    }

    if (is->aCodecCtx) {
        avcodec_free_context(&is->aCodecCtx);
        is->aCodecCtx = nullptr;
    }

    if (is->vCodecCtx) {
        avcodec_free_context(&is->vCodecCtx);
        is->vCodecCtx = nullptr;
    }

    if (is->swr_ctx) {
        swr_free(&is->swr_ctx);
        is->swr_ctx = nullptr;
    }

    if (is->sws_ctx) {
        sws_freeContext(is->sws_ctx);
        is->sws_ctx = nullptr;
    }
}

void DemuxThread::run()
{
    decodeLoop();
}

int DemuxThread::decodeLoop()
{
    AVPacket *packet = av_packet_alloc();

    while (true)
    {
        // 是否暂停播放
        if (m_stop) {
            log_println("request quit while decode_loop");
            break;
        }

        // 是否需要跳转
        if (m_playerCtx->seek_require) {
            int stream_index = -1;
            int64_t seek_pos = m_playerCtx->seek_pos;

            if (m_playerCtx->videoStream >= 0) {
                stream_index = m_playerCtx->videoStream;
            } else if (m_playerCtx->audioStream >= 0) {
                stream_index = m_playerCtx->audioStream;
            }

            if (stream_index >= 0) {
                seek_pos = av_rescale_q(seek_pos, AVRational{ 1, AV_TIME_BASE }, 
                    m_playerCtx->formatCtx->streams[stream_index]->time_base);
            }

            if (av_seek_frame(m_playerCtx->formatCtx, stream_index, seek_pos, m_playerCtx->seek_flags) < 0) {
                log_println("%s: error while seeking\n", m_playerCtx->filename);
            } else {
				if (m_playerCtx->audioStream >= 0) {
                    m_playerCtx->audio_queue.packetFlush();
                    m_playerCtx->flush_a_ctx = true;
				}
				if (m_playerCtx->videoStream >= 0) {
                    m_playerCtx->video_queue.packetFlush();
                    m_playerCtx->flush_v_ctx = true;
				}
            }

            m_playerCtx->seek_require = 0;
        }

        // 
        if (m_playerCtx->audio_queue.packetSize() > MAX_AUDIOQ_SIZE ||
            m_playerCtx->video_queue.packetSize() > MAX_VIDEOQ_SIZE) {
            SDL_Delay(10);
            continue;
        }

        // 
        if (av_read_frame(m_playerCtx->formatCtx, packet)) {
            log_println("av_read_frame error");
            break;
        }

        //
        if (packet->stream_index == m_playerCtx->videoStream) {
            m_playerCtx->video_queue.packetPut(packet);
        }
        else if (packet->stream_index == m_playerCtx->audioStream) {
            m_playerCtx->audio_queue.packetPut(packet);
        }
        else {
            av_packet_unref(packet);
        }

    }

    while (!m_stop)
    {
        SDL_Delay(100);
    }

    av_packet_free(&packet);

    SDL_Event event;
    event.type = FF_QUIT_EVENT;
    event.user.data1 = m_playerCtx;
    SDL_PushEvent(&event);

    return 0;
}

/**
 * @brief 
 * 
 * @param playerCtx 
 * @param mediaType 
 * @return int 
 */
int DemuxThread::streamOpen(FFmpegPlayerCtx *playerCtx, int mediaType)
{
    AVFormatContext *formatCtx = playerCtx->formatCtx;
    AVCodecContext codecCtx = NULL;
    AVCodec codec = NULL;

    /// @fn av_find_best_stream
    /// 查找最佳音视频流
    int stream_index = av_find_best_stream(formatCtx, (AVMediaType)mediaType, -1, -1, (const AVCodec **)&codec, 0);
    if (stream_index < 0 || stream_index >= (int)playerCtx->nb_streams) {
        log_println("Failed to find a stream in input file");
        return -1;
    }

    /// @fn avcodec_alloc_context3
    /// @brief 分配内存
    codecCtx = avcodec_alloc_context3(codec);
    /// @fn avcodec_parameters_to_context
    /// @brief 将流参数拷贝到解码器
    avcodec_parameters_to_context(codecCtx, formatCtx->streams[stream_index]->codecpar);

    if (avcode_open2(codecCtx, codec, NULL) < 0) {
        log_println("Failed to open codec for stream #%d", stream_index);
    }

    switch (codecCtx->codec_type) {
        case AVMEDIA_TYPE_AUDIO: {
            m_playerCtx->audioStream = stream_index;
            m_playerCtx->aCodecCtx = codecCtx;
            m_playerCtx->audio_st = formatCtx->streams[stream_index];
            m_playerCtx->swr_ctx = swr_alloc();
            
            av_opt_set_chlayout(m_playerCtx->swr_ctx, "in_chlayout", &codecCtx->ch_layout, 0);
            av_opt_set_int(m_playerCtx->swr_ctx, "in_sample_rate", codecCtx->sample_rate, 0);
            av_opt_set_sample_fmt(m_playerCtx->swr_ctx, "in_sample_fmt", codecCtx->sample_fmt, 0);

            AVChannelLayout outLayout;

            av_channel_layout_default(&outLayout, 2);

			av_opt_set_chlayout(m_playerCtx->swr_ctx, "out_chlayout", &outLayout, 0);
			av_opt_set_int(m_playerCtx->swr_ctx, "out_sample_rate", 48000, 0);
			av_opt_set_sample_fmt(m_playerCtx->swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
            swr_init(m_playerCtx->swr_ctx);
        } break;
        case AVMEDIA_TYPE_VIDEO: {
            m_playerCtx->videoStream = stream_index;
            m_playerCtx->vCodecCtx = codecCtx;
            m_playerCtx->video_st = formatCtx->streams[stream_index];
            m_playerCtx->frame_timer = (double) av_gettime() / 1000000.0;
            m_playerCtx->frame_last_delay = 40e-3;
            /// @fn sws_getContext
            /// @brief 
            m_playerCtx->sws_ctx = sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
                                                  codecCtx->width, codecCtx->height,
                                                  AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
        } break;
        default: break;
    }

    return 0;
}
