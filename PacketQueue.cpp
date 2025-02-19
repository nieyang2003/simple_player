#include "PacketQueue.h"
#include "log.h"

PacketQueue::PacketQueue()
{
    m_mutex = SDL_CreateMutex();
    m_cond = SDL_CreateCond();
}

int PacketQueue::packetPut(AVPacket *packet)
{
    debug_println("PacketQueue::packetPut");
    SDL_LockMutex(m_mutex);

    m_packets.push_back(*packet);
    m_size += packet->size;

    SDL_CondSignal(m_cond);
    SDL_UnlockMutex(m_mutex);

    return 0;
}

/**
 * @brief 从包队列中获取一个包
 * 
 * @param packet 包指针（已分配内存）
 * @param quit 是否处于停止状态
 * @return int 负数失败
 */
int PacketQueue::packetGet(AVPacket *packet, std::atomic<bool> &quit)
{   
    debug_println("PacketQueue::packetGet");
    int ret = 0;
    SDL_LockMutex(m_mutex);

    while (true)
    {
        if (!m_packets.empty()) {
            AVPacket& firstPacket = m_packets.front();

            m_size -= firstPacket.size;
            *packet = firstPacket;
            
            m_packets.erase(m_packets.begin());

            ret = 1;
            break;
        }
        else {
            SDL_CondWaitTimeout(m_cond, m_mutex, 500);
        }

        if (quit) {
            ret = -1;
            break;
        }
    }

    SDL_UnlockMutex(m_mutex);
    return ret;
}

/// @brief 清空队列
void PacketQueue::packetFlush()
{
    debug_println("PacketQueue::packetFlush");
    SDL_LockMutex(m_mutex);

    std::list<AVPacket>::iterator iter;
    for (iter = m_packets.begin(); iter != m_packets.end(); ++iter) {
        AVPacket& packet = *iter;
        av_packet_unref(&packet);
    }
    m_packets.clear();

    m_size = 0;

    SDL_UnlockMutex(m_mutex);
}

int PacketQueue::packetSize()
{
    return m_size;
}
