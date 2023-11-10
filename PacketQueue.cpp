#include "PacketQueue.h"

PacketQueue::PacketQueue()
{
    m_mutex = SDL_CreateMutex();
    m_cond = SDL_CreateCond();
}

int PacketQueue::packetPut(AVPacket *packet)
{
    SDL_LockMutex(m_mutex);

    m_packets.push_back(*packet);
    m_size += packet->size;

    SDL_CondSignal(m_cond);
    SDL_UnlockMutex(m_mutex);

    return 0;
}

int PacketQueue::packetGet(AVPacket *packet, std::atomic<bool> &quit)
{   
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
    return 0;
}

/**
 * @brief 清空队列
 * 
 */
void PacketQueue::packetFlush()
{
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
