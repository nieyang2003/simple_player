#include "DemuxThread.h"
#include "FFmpegPlayer.h"
#include "log.h"

DemuxThread::DemuxThread()
{
}

/**
 * @brief 初始化线程
 * 
 * @return int 
 */
int DemuxThread::initDemuxThread()
{
    AVFormatContext *formatCtx = NULL;
    /// @fn avformat_open_input
    /// @brief 打开文件并自动分配上下文
    if (avformat_open_input(&formatCtx, m_playerCtx->filename, NULL, NULL) != 0) {
        log_println("Failed to open input file");
        return -1;
    }

    m_playerCtx->formatCtx = formatCtx;

    /// @fn avformat_find_stream_info
    /// @brief 获取媒体文件的流信息
    if (avformat_find_stream_info(formatCtx, NULL) < 0) {
        log_println("avformat_find_stream_info failed");
        return -1;
    }

    /// @fn av_dump_file
    /// @brief 打印媒体文件信息
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
    if (m_playerCtx->formatCtx) {
        // 媒体文件上下文必须用函数关闭
        avformat_close_input(&m_playerCtx->formatCtx);
        m_playerCtx->formatCtx = nullptr;
    }

    if (m_playerCtx->aCodecCtx) {
        avcodec_free_context(&m_playerCtx->aCodecCtx);
        m_playerCtx->aCodecCtx = nullptr;
    }

    if (m_playerCtx->vCodecCtx) {
        avcodec_free_context(&m_playerCtx->vCodecCtx);
        m_playerCtx->vCodecCtx = nullptr;
    }

    if (m_playerCtx->swr_ctx) {
        swr_free(&m_playerCtx->swr_ctx);
        m_playerCtx->swr_ctx = nullptr;
    }

    if (m_playerCtx->sws_ctx) {
        sws_freeContext(m_playerCtx->sws_ctx);
        m_playerCtx->sws_ctx = nullptr;
    }
}

void DemuxThread::run()
{
    decodeLoop();
}

/**
 * @brief 解封装并将帧放入播放器上下文音视频帧队列中
 * 
 * @return int 
 */
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
            int stream_index = -1;  // 流索引
            int64_t seek_pos = m_playerCtx->seek_pos;   // seek位置，在seek_require之前被设置

            if (m_playerCtx->videoStream >= 0) {
                stream_index = m_playerCtx->videoStream;
            } else if (m_playerCtx->audioStream >= 0) {
                stream_index = m_playerCtx->audioStream;
            }

            if (stream_index >= 0) {
                /// @fn av_rescale_q
                /// @brief 将时间值从一个时基转换为另一个时基
                seek_pos = av_rescale_q(seek_pos, AVRational{ 1, AV_TIME_BASE}, 
                    m_playerCtx->formatCtx->streams[stream_index]->time_base);
            }

            /// @fn av_seek_frame
            /// @brief 在媒体文件内进行跳转
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

        // 队列数据大于阈值，延时10ms
        if (m_playerCtx->audio_queue.packetSize() > MAX_AUDIOQ_SIZE ||
            m_playerCtx->video_queue.packetSize() > MAX_VIDEOQ_SIZE) {
            SDL_Delay(10);
            continue;
        }

        /// @fn av_read_frame
        /// @brief 从媒体文件中读取音视频帧
        if (av_read_frame(m_playerCtx->formatCtx, packet)) {
            log_println("av_read_frame error");
            break;
        }

        // 根据类型放入对应队列
        if (packet->stream_index == m_playerCtx->videoStream) {
            m_playerCtx->video_queue.packetPut(packet);
        }
        else if (packet->stream_index == m_playerCtx->audioStream) {
            m_playerCtx->audio_queue.packetPut(packet);
        }
        else {
            // 释放帧数据
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
 * @brief 打开音频或视频解码器
 * 
 * @param playerCtx 
 * @param mediaType 
 * @return int 
 */
int DemuxThread::streamOpen(FFmpegPlayerCtx *playerCtx, int mediaType)
{
    AVFormatContext *formatCtx = playerCtx->formatCtx;
    AVCodecContext *codecCtx = NULL;
    AVCodec *codec = NULL;

    /// @fn av_find_best_stream
    /// 查找最佳音视频流，解码器仍未打开
    int stream_index = av_find_best_stream(formatCtx, (AVMediaType)mediaType, -1, -1, (const AVCodec **)&codec, 0);
    if (stream_index < 0 || stream_index >= (int)formatCtx->nb_streams) {
        log_println("Failed to find a stream in input file");
        return -1;
    }

    /// @fn avcodec_alloc_context3
    /// @brief 分配解码器上下文（但是大部分信息未填充，调用avcodec_parameters_to_context函数）
    codecCtx = avcodec_alloc_context3(codec);
    /// @fn avcodec_parameters_to_context
    /// @brief 将流参数拷贝到解码器
    avcodec_parameters_to_context(codecCtx, formatCtx->streams[stream_index]->codecpar);

    /// @fn avcode_open2
    /// @brief 打开解码器
    if (avcode_open2(codecCtx, codec, NULL) < 0) {
        log_println("Failed to open codec for stream #%d", stream_index);
    }

    // 根据解码类型设置播放器上下文信息
    /// @todo
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
