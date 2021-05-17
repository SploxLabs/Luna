#pragma once
#include <dsound.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <wrl.h>

#include "Debuger.h"

using namespace Microsoft::WRL;

class DSAudioEngine
{
public:
	DSAudioEngine();
	DSAudioEngine(HWND WINDOW_HANDLE);
	~DSAudioEngine();
	void Init(); //init direct sound and primary buffer
	void LoadSounds();
	void loadWavFileToBuffer(char* filename, IDirectSoundBuffer8** Buffer);
	void CreateSoundBuffer(LPDIRECTSOUNDBUFFER8* Sound_Buffer);
	HWND hwnd;

	void PlayWaveFile();
	void PlayWaveFile(char* Name);

	struct Sound {
		Sound() {
			name = (char*)"name_me";
			sound_buffer = nullptr;
		}
		char* name;
		IDirectSoundBuffer8* sound_buffer;
	};

	IDirectSoundBuffer8* ambience;
	IDirectSoundBuffer8* rcs;
	IDirectSoundBuffer8* thruster;
	IDirectSoundBuffer8* alarm;

private:

	struct WaveHeaderType
	{
		char chunkId[4];
		unsigned long chunkSize;
		char format[4];
		char subChunkId[4];
		unsigned long subChunkSize;
		unsigned short audioFormat;
		unsigned short numChannels;
		unsigned long sampleRate;
		unsigned long bytesPerSecond;
		unsigned short blockAlign;
		unsigned short bitsPerSample;
		char dataChunkId[4];
		unsigned long dataSize;
	};

	
	void PlaySpecificSound(Sound SOUND);
	void PlaySpecificSound(IDirectSoundBuffer8* BUFFER_PTR);
	Sound FindSound(char* NAME);

	

	IDirectSound8* sound_device;
	IDirectSoundBuffer* primary_sound_buffer;
	std::vector<Sound> sounds;
};