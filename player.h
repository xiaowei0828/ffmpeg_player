
#include "VideoContext.h"
#include <string>
#include <memory>

#define DEFAULT_AUDIO_AND_VIDEO 1

class player
{
public:
	player();
	~player();

	void play();

	void pause();

	void loadLocalFile(std::string filename);

	void loadStreamingFile();

	bool init();

	bool initVideoDevice(std::shared_ptr<VideoContext> pVC);

	bool initAudioDevice(std::shared_ptr<VideoContext> pVC);

	static void fill_audio(void* user_data, uint8_t* stream, int len);

	static int readVideo(void* user_data);

	static uint32_t showVideo(uint32_t interval, void* param);

	static int readFrame(void* user_data);

	void freeMem();

	void load();

private:
	std::shared_ptr<VideoContext> m_pVC;
	std::string m_localFileName;
};