
#include "player.h"

int main(int arc, char* argv[])
{
	player test;
	test.loadLocalFile("D:\\vs2013_projects\\simplest_ffmpeg_player\\ffmpeg_player_real\\test.mp4");
	test.load();
	SDL_Event evt;
	while (1)
	{
		SDL_WaitEvent(&evt);
		switch (evt.type)
		{
		case SDL_KEYDOWN:
		{
			switch (evt.key.keysym.sym)
			{
			case SDLK_p:
			{
				printf("Print a Screen shoot\n");
			}break;
			}
		}break;
		case SDL_MOUSEMOTION:
			break;
		case SDL_MOUSEBUTTONDOWN:
			break;
		case SDL_QUIT:
		{
			test.freeMem();
			exit(0);
		}
		}
	}
	return 0;
}