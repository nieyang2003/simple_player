#include "AudioDecodeThread.h"

#include "FFmpegPlayer.h"
#include "log.h"


void AudioDecodeThread::getAudioData(unsigned char *stream, int length)
{
    // 解码器未就绪，写入空数据
    if (!m_playerCtx->aCodecCtx || m_playerCtx->pause_state = PAUSE)
    {
        memset(stream, 0, length);
        return;
    }
    
    int len, audio_size;
    double pts;

    while (length > 0)
    {
        if (m_playerCtx->audio_buf_index >= m_playerCtx->audio_buf_size)
        {
            audio_size = audio_decode_frame(m_playerCtx, &pts);
            if (audio_size < 0)
            {
                // 解码出错
                m_playerCtx->audio_buf_size = 1024;
                memset(m_playerCtx->audio_buf, 0, m_playerCtx->audio_buf_size);
            }
            else
            {
                m_playerCtx->audio_buf_size = audio_size;
            }
            m_playerCtx->audio_buf_index = 0;
        }

        len = m_playerCtx->audio_buf_size - m_playerCtx->audio_buf_index;
        if (len > length)
            len = length;

        memcpy(stream, (uint8_t*)m_playerCtx->audio_buf + m_playerCtx->audio_buf_index, len);
        length -= len;
        stream += len;
        m_playerCtx->audio_buf_index += len;
    }
}

void AudioDecodeThread::run()
{
    // @todo
}

/**
 * @brief 
 * 
 * @param playerCtx 播放器上下文指针
 * @param ppts 播放时间戳PTS指针
 * @return int 
 */
int AudioDecodeThread::audio_decode_frame(FFmpegPlayerCtx *playerCtx, double *ppts)
{
    int len1 = 0, data_size = 0, n = 0;
    AVPacket* packet = m_playerCtx->audio_packet;
    double pts;
    int ret = 0;

    while (true)
    {
        while (m_playerCtx->audio_packet_size > 0)
        {
            /// @fn avcodec_send_packet
            /// @brief 将待解码数据包发送给解码器解码
            /// @return int，0表示成功，负数失败
            ret = avcodec_send_packet(m_playerCtx->aCodecCtx, packet);
            if (ret != 0)
            {
                m_playerCtx->audio_packet_size = 0;
                break;
            }

            /// @fn av_frame_unref
            /// @brief 减少引用，为0释放内存
            av_frame_unref(m_playerCtx->audio_frame);
            /// @fn avcodec_receive_frame
            /// @brief 从解码器接受解码后的视频或音频帧
            /// @return int，成功0，无法接受负数
            ret = avcodec_receive_frame(m_playerCtx->aCodecCtx, m_playerCtx->audio_frame);
			if (ret != 0)
			{
				m_playerCtx->audio_packet_size = 0;
				break;
			}

            if (ret == 0)
            {
                /// @fn swr_get_out_samples
                /// @brief 查询音频重采样器中输出缓冲区中的样本数
                // 确定样本数上限，分配合适内存
                int upper_bound_samples = swr_get_out_samples(m_playerCtx->swr_ctx, m_playerCtx->audio_frame->nb_samples);

                uint8_t* out[4] = { 0 };
                out[0] = (uint8_t*)av_malloc(upper_bound_samples * 2 * 2); // 每个样本占用两字节，双声道

                /// @fn swr_convert
                /// @param s 重采样结构体指针
                /// @param out 输出缓冲区
                /// @param out_count 缓冲区样本数
                /// @param in 输入缓冲区
                /// @param in_count 输入缓冲区样本数
                /// @brief 执行音频重采样操作
                /// @return int，实际样本数，负数出错
                // 重采样以使音频可以被SDL2播放
                int samples = swr_convert(m_playerCtx->swr_ctx, out, upper_bound_samples,
                    (const uint8_t**)m_playerCtx->audio_frame->data, m_playerCtx->audio_frame->nb_samples);

                if (samples > 0)
                {
                    memcpy(m_playerCtx->audio_buf, out[0], samples * 2 * 2);
                }

                av_free(out[0]);

                data_size = samples * 2 * 2;
            }

            len1 = packet->size;
            m_playerCtx->audio_packet_data += len1;
            m_playerCtx->audio_packet_size -= len1;

            if (data_size <= 0)
            {
                continue;
            }

            // 计算音频播放时间戳
            *ppts = m_playerCtx->audio_clock;   // 音频时钟当前时间戳
            n = 2 * m_playerCtx->aCodecCtx->ch_layout.nb_channels;  // 采样点总数
            m_playerCtx->audio_clock += (double)data_size / (double)(n * (m_playerCtx->aCodecCtx->sample_rate));

            return data_size;
        }

        if (m_stop)
        {
			log_println("request quit while decode audio");
			return -1;
        }

        if (m_playerCtx->flush_a_ctx) {
            m_playerCtx->flush_a_ctx = false;
			log_println("avcodec_flush_buffers(aCodecCtx) for seeking");
            /// @fn avcodec_flush_buffers
            /// @brief 清空解码器内部缓冲区
			avcodec_flush_buffers(m_playerCtx->aCodecCtx);
			continue;
        }

        av_packet_unref(packet);

        // 获取下一个数据包
        if (m_playerCtx->audio_queue.packetGet(packet, m_stop) < 0)
        {
            return -1;
        }

        m_playerCtx->audio_packet_data = packet->data;
        m_playerCtx->audio_packet_size = packet->size;

		if (packet->pts != AV_NOPTS_VALUE) {    // PTS是否未知
            // 将time_base为单位的时间转换为以秒为单位的时间
            m_playerCtx->audio_clock = av_q2d(m_playerCtx->audio_st->time_base) * packet->pts;
		}
    }

    return 0;
}
