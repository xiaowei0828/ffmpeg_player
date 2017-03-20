
#include <stdint.h>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "SDL1-2/SDL.h"
}

#include <list>

struct VideoContext
{
	
	AVFormatContext* pFormatCtx;
	int nbStreams;
	AVStream** streams;
	AVCodecContext* pVideoCodecCtx;
	int videoindex;
	AVCodecContext* pAudioCodecCtx;
	int audioindex;
	AVCodecContext* pSubCodecCtx;
	int subindex;

	int64_t videoClockTime;
	int64_t audioClockTime;

	int nbChannels;
	AVCodecID audioCodecID;
	AVCodecID videoCodecID;
	AVCodec* pVideoDec;
	AVCodec* pAudioDec;

	SDL_Surface* pScrren;
	SDL_Overlay* pBmp;

	SwrContext* pAu_covert;
	int out_buffer_size;
	uint8_t* out_buffer;

	int frameEof;

	std::list<AVPacket*> audioList;
	std::list<AVPacket*> videoList;
	std::list<AVPacket*> subList;

	AVFrame* pDecodeVideoFrame;

	SDL_mutex* pVideoListMutex;
	SDL_mutex* pAudioListMutex;
	SDL_mutex* pSubListMutex;
	SDL_mutex* videoclockMutex;
	SDL_mutex* audioclockMutex;
	SDL_cond* pVideoListCond;
	SDL_cond* pAudioListCond;
	SDL_cond* pSubListCond;

	SDL_Thread* read_tid;
	SDL_Thread* videoPlay_tid;

	VideoContext()
	{
		pFormatCtx = avformat_alloc_context();
		nbStreams = 0;
		streams = NULL;
		pVideoCodecCtx = NULL;
		videoindex = 0;
		pAudioCodecCtx = NULL;
		audioindex = 0;
		pSubCodecCtx = NULL;

		videoClockTime = 0;
		audioClockTime = 0;
		pVideoDec = NULL;
		pAudioDec = NULL;
		pScrren = NULL;
		pBmp = NULL;
		pAu_covert = NULL;
		out_buffer = NULL;
		out_buffer_size = 0;
		pDecodeVideoFrame = NULL;
		nbChannels = 0;
		subindex = 0;
		frameEof = 0;
	}
};