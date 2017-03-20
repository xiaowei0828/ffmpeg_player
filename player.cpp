
#include "player.h"
#include <windows.h>

player::player() :m_pVC(new VideoContext())
{
}

player::~player()
{

}

void player::play()
{

}

void player::pause()
{

}

void player::loadLocalFile(std::string filename)
{
	m_localFileName = filename;
	init();
}

void player::loadStreamingFile()
{

}

bool player::init()
{
	av_register_all();
	SDL_Init(SDL_INIT_EVERYTHING);
	printf("%s\n", m_localFileName.c_str());
	if (avformat_open_input(&(m_pVC->pFormatCtx), m_localFileName.c_str(), NULL, NULL) < 0)
	{
		printf("open input file Error\n");
		return false;
	}

	if (avformat_find_stream_info(m_pVC->pFormatCtx, NULL) < 0)
	{
		printf("Could not find stream information\n");
	}
	av_dump_format(m_pVC->pFormatCtx, 0, m_localFileName.c_str(), 0);

	m_pVC->nbStreams = m_pVC->pFormatCtx->nb_streams;
	m_pVC->streams = new AVStream*[m_pVC->nbStreams];
	for (int i = 0; i < m_pVC->nbStreams; i++)
	{
		m_pVC->streams[i] = m_pVC->pFormatCtx->streams[i];
	}

	//默认情况下之取第一个audio和video作为播放源
	if (DEFAULT_AUDIO_AND_VIDEO)
	{
		for (int i = 0; i < m_pVC->nbStreams; i++)
		{
			if (m_pVC->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				if (m_pVC->pVideoCodecCtx == NULL)
				{
					m_pVC->pVideoCodecCtx = m_pVC->streams[i]->codec;
					m_pVC->videoindex = i;
				}
			}
			else if (m_pVC->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
			{
				if (m_pVC->pAudioCodecCtx == NULL)
				{
					m_pVC->pAudioCodecCtx = m_pVC->streams[i]->codec;
					m_pVC->audioindex = i;
				}
			}
			else if (m_pVC->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE)
			{
				if (m_pVC->pSubCodecCtx == NULL)
				{
					m_pVC->pSubCodecCtx = m_pVC->streams[i]->codec;
					m_pVC->subindex = i;
				}
			}
		}
	}
	else
	{

	}
	m_pVC->nbChannels = m_pVC->pAudioCodecCtx->channels;
	m_pVC->audioCodecID = m_pVC->pAudioCodecCtx->codec_id;
	m_pVC->videoCodecID = m_pVC->pVideoCodecCtx->codec_id;

	m_pVC->pVideoDec = avcodec_find_decoder(m_pVC->videoCodecID);
	if (m_pVC->pVideoDec == NULL)
	{
		printf("Can't find video dec\n");
		return false;
	}
	if (avcodec_open2(m_pVC->pVideoCodecCtx, m_pVC->pVideoDec, NULL) < 0)
	{
		return false; // Could not open codec
	}
	m_pVC->pAudioDec = avcodec_find_decoder(m_pVC->audioCodecID);
	if (m_pVC->pAudioDec == NULL)
	{
		printf("Can't find video dec\n");
		return false;
	}
	if (avcodec_open2(m_pVC->pAudioCodecCtx, m_pVC->pAudioDec, NULL) < 0)
	{
		return false; // Could not open codec
	}

	initVideoDevice(m_pVC);
	initAudioDevice(m_pVC);

	m_pVC->pVideoListMutex = SDL_CreateMutex();
	m_pVC->pAudioListMutex = SDL_CreateMutex();
	m_pVC->pSubListMutex = SDL_CreateMutex();
	m_pVC->videoclockMutex = SDL_CreateMutex();
	m_pVC->audioclockMutex = SDL_CreateMutex();
	m_pVC->pVideoListCond = SDL_CreateCond();
	m_pVC->pAudioListCond = SDL_CreateCond();
	m_pVC->pSubListCond = SDL_CreateCond();

	return true;

}

bool player::initVideoDevice(std::shared_ptr<VideoContext> pVC)
{
	pVC->videoClockTime = pVC->pFormatCtx->streams[pVC->videoindex]->start_time;
	int flags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL;
	flags |= SDL_RESIZABLE;
	pVC->pScrren = SDL_SetVideoMode(pVC->pVideoCodecCtx->width, pVC->pVideoCodecCtx->height, 0, flags);
	if (!pVC->pScrren)
	{
		printf("SDL: could not set video mode - exiting\n");
		return false;
	}
	//AVRational frame_rate = av_guess_frame_rate(pVC->pFormatCtx, pVC->pFormatCtx->streams[pVC->videoindex], NULL);
	//pVC->videoDuration = av_q2d(pVC->pFormatCtx->streams[pVC->videoindex]->r_frame_rate)*av_q2d(pVC->pVideoCodecCtx->time_base);
	SDL_WM_SetCaption("MAOYIKAI PLAYER ^_^", NULL);
	if (pVC->pBmp)
	{
		SDL_FreeYUVOverlay(pVC->pBmp);
	}
	pVC->pBmp = SDL_CreateYUVOverlay(pVC->pVideoCodecCtx->width,
		pVC->pVideoCodecCtx->height,
		SDL_IYUV_OVERLAY,
		pVC->pScrren);

	return true;
}

bool player::initAudioDevice(std::shared_ptr<VideoContext> pVC)
{
	pVC->audioClockTime = pVC->pFormatCtx->streams[pVC->audioindex]->start_time;
	//pVC->audioDuration = 1024.0/pVC->pAudioCodecCtx->sample_rate;
	//pVC->audioDuration = av_q2d(pVC->pFormatCtx->streams[pVC->audioindex]->r_frame_rate)*av_q2d(pVC->pFormatCtx->streams[pVC->audioindex]->time_base);
	SDL_AudioSpec wantspec, spec;
	int m_dwBitsPerSample = 0;
	switch (pVC->pAudioCodecCtx->sample_fmt)
	{
	case AV_SAMPLE_FMT_U8:
	{
		m_dwBitsPerSample = 8;
	}break;
	case AV_SAMPLE_FMT_S16:
	{
		m_dwBitsPerSample = 16;
	}break;
	case AV_SAMPLE_FMT_S32:
	{
		m_dwBitsPerSample = 32;
	}break;
	case AV_SAMPLE_FMT_FLTP:
	{
		m_dwBitsPerSample = 32;
	}break;
	default:
	{
		m_dwBitsPerSample = 0;
	}break;
	}
	wantspec.freq = pVC->pAudioCodecCtx->sample_rate;
	switch (m_dwBitsPerSample)
	{
	case 8:
		wantspec.format = AUDIO_S8;
		break;
	case 16:
		wantspec.format = AUDIO_S16SYS;
		break;
	default:
		wantspec.format = AUDIO_S16SYS;
		break;
	}
	wantspec.channels = pVC->nbChannels;
	wantspec.silence = 0;
	wantspec.samples = pVC->pAudioCodecCtx->frame_size;
	wantspec.callback = fill_audio;
	wantspec.userdata = &(*pVC);

	if (SDL_OpenAudio(&wantspec, &spec) < 0)
	{
		fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
		return false;
	}

	if (spec.format != AUDIO_S16SYS)
	{
		fprintf(stderr, "SDL advised audio format %d is not supported!\n", spec.format);
		return false;
	}
	pVC->pAu_covert = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, pVC->pAudioCodecCtx->sample_rate, pVC->pAudioCodecCtx->channel_layout,
		pVC->pAudioCodecCtx->sample_fmt, pVC->pAudioCodecCtx->sample_rate, 0, NULL);
	swr_init(pVC->pAu_covert);
	pVC->out_buffer_size = av_samples_get_buffer_size(NULL, pVC->pAudioCodecCtx->channels, pVC->pAudioCodecCtx->frame_size, *(pVC->pAudioDec->sample_fmts), 1);
	pVC->out_buffer = (uint8_t *)av_malloc(pVC->out_buffer_size);
	memset(pVC->out_buffer, 0, pVC->out_buffer_size);
	SDL_PauseAudio(0);
	return true;
}

void player::fill_audio(void* user_data, Uint8* stream, int len)
{
	VideoContext* pVc = (VideoContext*)user_data;
	SDL_LockMutex(pVc->pAudioListMutex);
	if (pVc->audioList.empty())
	{
		if (pVc->frameEof == 1)
		{
			printf("audio is play over\n");
			SDL_UnlockMutex(pVc->pAudioListMutex);

			if (pVc->videoList.empty())
			{
				SDL_Event evt;
				evt.type = SDL_QUIT;
				SDL_PushEvent(&evt);
			}
			SDL_CloseAudio();
			return;
		}
		SDL_UnlockMutex(pVc->pAudioListMutex);
		SDL_CondWait(pVc->pAudioListCond, NULL);
		SDL_LockMutex(pVc->pAudioListMutex);
	}
	AVPacket *tmppkt = pVc->audioList.front();
	if (!tmppkt)
	{
		printf("not include availble audio data\n");
		return;
	}
	SDL_UnlockMutex(pVc->pAudioListMutex);
	AVFrame* pFrame = NULL;
	int got_picture = 0, ret = 0;
	pFrame = avcodec_alloc_frame();
	ret = avcodec_decode_audio4(pVc->pAudioCodecCtx, pFrame, &got_picture, tmppkt);
	if (ret < 0)
	{
		printf("Error in decoding audio frame.\n");
		return;
	}
	printf("-------audio clock time is %f\n", pVc->audioClockTime);
	if (got_picture > 0)
	{
		//pVc->audioClockTime = tmppkt->pts * av_q2d(pVc->pFormatCtx->streams[pVc->audioindex]->time_base);
		pVc->audioClockTime += pVc->pAudioCodecCtx->frame_size*1.0 / pVc->pAudioCodecCtx->sample_rate; //tmppkt->duration*av_q2d(pVc->pFormatCtx->streams[pVc->audioindex]->time_base);
		swr_convert(pVc->pAu_covert, &(pVc->out_buffer), pVc->out_buffer_size, (const uint8_t **)pFrame->data, pFrame->nb_samples);
	}
	else
	{
		memset(pVc->out_buffer, 0, pVc->out_buffer_size);
		SDL_LockMutex(pVc->pAudioListMutex);
		pVc->audioList.pop_front();
		SDL_UnlockMutex(pVc->pAudioListMutex);
		return;
	}

	memcpy(stream, (uint8_t*)(pVc->out_buffer), len);
	memset(pVc->out_buffer, 0, pVc->out_buffer_size);

	SDL_LockMutex(pVc->pAudioListMutex);
	pVc->audioList.pop_front();
	SDL_UnlockMutex(pVc->pAudioListMutex);
	av_free_packet(tmppkt);
	avcodec_free_frame(&pFrame);

}

int player::readVideo(void* user_data)
{
	VideoContext* pVc = (VideoContext*)user_data;
	AVFrame * pFrame = NULL;
	pFrame = avcodec_alloc_frame();
	pVc->pDecodeVideoFrame = pFrame;
	int frameFinished = 0, ret = 0;
	while (1)
	{
		SDL_LockMutex(pVc->pVideoListMutex);
		if (pVc->videoList.empty())
		{
			if (pVc->frameEof == 1)
			{
				printf("video is play over\n");
				SDL_UnlockMutex(pVc->pVideoListMutex);
				return 0;
			}
			SDL_UnlockMutex(pVc->pVideoListMutex);
			SDL_CondWait(pVc->pVideoListCond, NULL);
			SDL_LockMutex(pVc->pVideoListMutex);
		}
		AVPacket *tmppkt = pVc->videoList.front();
		if (!tmppkt)
		{
			printf("can't get packet data of video\n");
			return 0;
		}
		SDL_UnlockMutex(pVc->pVideoListMutex);
		ret = avcodec_decode_video2(pVc->pVideoCodecCtx, pFrame, &frameFinished, tmppkt);
		if (ret < 0)
		{
			continue;
		}

		printf("++++++++video clock time is %f\n", pVc->videoClockTime);

		if (frameFinished)
		{
			//pVc->videoClockTime = tmppkt->pts * av_q2d(pVc->pFormatCtx->streams[pVc->videoindex]->time_base);
			pVc->videoClockTime += 1.0 / av_q2d(pVc->pFormatCtx->streams[pVc->videoindex]->r_frame_rate);//tmppkt->duration * av_q2d(pVc->pFormatCtx->streams[pVc->videoindex]->time_base);
			AVFrame * pOutputFrame = NULL;
			struct SwsContext * img_convert_ctx;
			pOutputFrame = avcodec_alloc_frame();

			SDL_LockYUVOverlay(pVc->pBmp);

			pOutputFrame->data[0] = pVc->pBmp->pixels[0];
			pOutputFrame->data[1] = pVc->pBmp->pixels[1];
			pOutputFrame->data[2] = pVc->pBmp->pixels[2];

			pOutputFrame->linesize[0] = pVc->pBmp->pitches[0];
			pOutputFrame->linesize[1] = pVc->pBmp->pitches[1];
			pOutputFrame->linesize[2] = pVc->pBmp->pitches[2];

			img_convert_ctx = sws_getContext(pFrame->width, pFrame->height, pVc->pFormatCtx->streams[pVc->videoindex]->codec->pix_fmt,
				pVc->pVideoCodecCtx->width, pVc->pVideoCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
			if (img_convert_ctx == NULL)
			{
				fprintf(stderr, "Cannot initialize the conversion context!\n");
				getchar();
			}
			// 将图片转换为RGB格式
			sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0,
				pFrame->height, pOutputFrame->data, pOutputFrame->linesize);

			SDL_UnlockYUVOverlay(pVc->pBmp);

			double time = 0;
			if ((pVc->videoClockTime - pVc->audioClockTime) * 1000 > 0)
			{
				time = (pVc->videoClockTime - pVc->audioClockTime) * 1000;
			}
			else if ((pVc->videoClockTime - pVc->audioClockTime) * 1000 <= 0)
			{
				time = 0;
			}
			Sleep(time);
			SDL_AddTimer(0, showVideo, &(*pVc));
			avcodec_free_frame(&pOutputFrame);
		}
		else
		{
			printf("can't decode frame , lost frame\n");
			pVc->videoClockTime += 1.0 / av_q2d(pVc->pFormatCtx->streams[pVc->videoindex]->r_frame_rate);
			//pVc->videoClockTime += tmppkt->duration*av_q2d(pVc->pFormatCtx->streams[pVc->videoindex]->time_base);//pVc->videoDuration;
		}
		av_free_packet(tmppkt);
		//avcodec_free_frame(&pFrame);
		SDL_LockMutex(pVc->pVideoListMutex);
		pVc->videoList.pop_front();
		SDL_UnlockMutex(pVc->pVideoListMutex);
	}
}

Uint32 player::showVideo(Uint32 interval, void *param)
{
	VideoContext *data = (VideoContext*)param;
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = data->pVideoCodecCtx->width;
	rect.h = data->pVideoCodecCtx->height;
	SDL_DisplayYUVOverlay(data->pBmp, &rect);
	return 0;
}

int player::readFrame(void* user_data)
{
	VideoContext* pVc = (VideoContext*)user_data;
	while (1)
	{
		AVPacket *pkt = new AVPacket;
		av_init_packet(pkt);
		if (av_read_frame(pVc->pFormatCtx, pkt))
		{
			printf("read frame error or End of File\n");
			pVc->frameEof = 1;
			return 0;
		}
		if (pkt->stream_index == pVc->videoindex)
		{
			SDL_LockMutex(pVc->pVideoListMutex);
			pVc->videoList.push_back(pkt);
			SDL_CondSignal(pVc->pVideoListCond);
			SDL_UnlockMutex(pVc->pVideoListMutex);
		}
		else if (pkt->stream_index == pVc->audioindex)
		{
			SDL_LockMutex(pVc->pAudioListMutex);
			pVc->audioList.push_back(pkt);
			SDL_CondSignal(pVc->pAudioListCond);
			SDL_UnlockMutex(pVc->pAudioListMutex);
		}
		else if (pkt->stream_index == pVc->subindex)
		{
			SDL_LockMutex(pVc->pSubListMutex);
			pVc->subList.push_back(pkt);
			SDL_UnlockMutex(pVc->pSubListMutex);
		}
	}
}

void player::freeMem()
{
	avcodec_free_frame(&(m_pVC->pDecodeVideoFrame));
}

void player::load()
{
	m_pVC->read_tid = SDL_CreateThread(readFrame, &(*m_pVC));
	m_pVC->videoPlay_tid = SDL_CreateThread(readVideo, &(*m_pVC));
}