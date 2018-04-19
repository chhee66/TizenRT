#include <tinyara/config.h>
#include <string.h>
#include "Decoder.h"
#include <debug.h>

#define PV_SUCCESS 0
#define PV_FAILURE -1

namespace media {

Decoder::Decoder()
{
#ifdef CONFIG_AUDIO_CODEC
	memset(&mPlayer, 0, sizeof(mPlayer));
	if (pv_player_init(&mPlayer, CONFIG_AUDIO_CODEC_RINGBUFFER_SIZE, this, _configFunc, nullptr, nullptr) !=
		PV_SUCCESS) {
		meddbg("Error! pv_player_init failed!\n");
	}
#endif
}

Decoder::Decoder(const Decoder* source)
{
#ifdef CONFIG_AUDIO_CODEC
	mPlayer = source->mPlayer;
#endif
}

Decoder::~Decoder()
{
#ifdef CONFIG_AUDIO_CODEC
	if (pv_player_finish(&mPlayer) != PV_SUCCESS) {
		meddbg("Error! pv_player_finish failed!\n");
	}
#endif
}

size_t Decoder::pushData(unsigned char* buf, size_t size)
{
#ifdef CONFIG_AUDIO_CODEC
	size_t rmax = pv_player_dataspace(&mPlayer);
	if (size > rmax) {
		meddbg("Error!! data is larger than rmax\n");
		size = rmax;
	}

	return pv_player_pushdata(&mPlayer, buf, size);
#endif
	return 0;
}

bool Decoder::getFrame(unsigned char* buf, size_t* size, unsigned int* sampleRate, unsigned short* channels)
{
#ifdef CONFIG_AUDIO_CODEC
	if (mPlayer.audio_type == 0) {
		mPlayer.audio_type = _get_audio_type(mPlayer.rbsp);
		medvdbg("audio_type %d\n", mPlayer.audio_type);

		if (_init_decoder(&mPlayer) != 0) {
			meddbg("Error! _init_decoder failed!\n");
			return false;
		}
	}

	if (_get_frame(&mPlayer)) {
		pcm_data_t pcm;
		if (_frame_decoder(&mPlayer, &pcm) == 0) {
			*size = pcm.length * sizeof(short);
			*sampleRate = pcm.samplerate;
			*channels = pcm.channels;
			memcpy(buf, pcm.samples, *size);
			return true;
		}
	}
#endif
	return false;
}

bool Decoder::empty()
{
#ifdef CONFIG_AUDIO_CODEC
	return pv_player_dataspace_is_empty(&mPlayer);
#endif
	return false;
}

size_t Decoder::getDataSpace()
{
#ifdef CONFIG_AUDIO_CODEC
	return pv_player_dataspace(&mPlayer);
#endif
	return 0;
}

#ifdef CONFIG_AUDIO_CODEC
int Decoder::_configFunc(void* user_data, int audio_type, void* dec_ext)
{
	/* To-do: Below buffer size and channel count must be calculated correctly. */
	static const int TARGET_SOUND_TRACK = 2;
	static uint8_t inputBuf[2048];
	static int16_t outputBuf[2048];

	if (audio_type == type_mp3) {
		tPVMP3DecoderExternal* mp3_ext = (tPVMP3DecoderExternal*)dec_ext;
		mp3_ext->equalizerType = flat;
		mp3_ext->crcEnabled = false;
		mp3_ext->pInputBuffer = inputBuf;
		mp3_ext->pOutputBuffer = outputBuf;
		mp3_ext->outputFrameSize = sizeof(outputBuf) / sizeof(int16_t);
		return 0;
	} else if (audio_type == type_aac) {
		tPVMP4AudioDecoderExternal* aac_ext = (tPVMP4AudioDecoderExternal*)dec_ext;
		aac_ext->outputFormat = OUTPUTFORMAT_16PCM_INTERLEAVED;
		aac_ext->desiredChannels = TARGET_SOUND_TRACK;
		aac_ext->pInputBuffer = inputBuf;
		aac_ext->pOutputBuffer = outputBuf;
		aac_ext->aacPlusEnabled = 1;
		return 0;
	}

	meddbg("Error! Not supported audio format!\n");
	return -1;
}
#endif

} // namespace media