/**
 * @file main.cpp
 * @author nieyang (nieyang2003@qq.com)
 * @brief 
 * @version 0.1
 * @date 2023-11-09
 * 
 * 
 */

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
}
#endif

#include "RenderView.h"
#include "SDLApp.h"
#include "Timer.h"
#include "log.h"

#include <functional>

#include "AudioPlay.h"
#include "FFmpegPlayer.h"

struct RenderPairData
{
	RenderItem* item = nullptr;
	RenderView* view = nullptr;
};

static void FN_DecodeImage_Cb(unsigned char* data, int w, int h, void* userdata)
{
	RenderPairData* cbData = (RenderPairData*)data;
	if (!cbData->item)
	{
		cbData->item = cbData->view->createRGB24Texture(w, h);
	}

	cbData->view->updateTexture(cbData->item, data, h);
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		log_println(usage: % s media_file_path", ". / ffmpeg_simple_palyer");
			return -1;
	}

	SDLApp sdlapp;

	RenderView view;
	view.initSDL();

	Timer timer;
	std::function<void()> cb = bind(&RenderView::onRefresh, &view);
	timer.start(&cb, 30);

	RenderPairData *cbData = new RenderPairData;
	cbData->view = view;

	FFmpegPlayer player;
	player.setFilePath(arvg[1]);
	player.setImageCb(FN_DecodeImage_Cb, cbData);
	if (player.initPlayer() != 0) {
		return -1;
	}

	log_println("FFmpegPlayer init success");

	player.start();

	return sdlapp.exec();
}
