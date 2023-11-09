/**
 * @file PacketQueue.h
 * @author nieyang (nieyang2003@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-11-09
 *
 *
 */
#pragma once

#include <list>
#include <atomic>

#ifdef __cplusplus
extern "C" {
#include <libavcodec/avcodec.h>
#include <SDL.h>
}
#endif

class PacketQueue
{
public:
	PacketQueue();

	int packetPut(AVPacket *packet);

	int packetGet(AVPacket* packet, std::atomic<bool>& quit);

	void packetFlush();

	int packetSize();

private:
	std::list<AVPacket> m_packets;
	std::atomic<int> m_size = 0;
	SDL_mutex* m_mutex = nullptr;
	SDL_cond* m_cond = nullptr;
};