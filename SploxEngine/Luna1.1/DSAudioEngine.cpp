#include "DSAudioEngine.h"

DSAudioEngine::DSAudioEngine() {
	//do-nothing
}

DSAudioEngine::DSAudioEngine(HWND WINDOW_HANDLE) {
	hwnd = WINDOW_HANDLE;
	sounds.clear();
	sound_device = 0;
	primary_sound_buffer = 0;
	ambience = 0;
	rcs = 0;
	thruster = 0;
	alarm = 0;

	Init();
}

DSAudioEngine::~DSAudioEngine(){
}

void DSAudioEngine::Init() {
	/* Create the Sound Device to the Defualt sound device of the computer*/
	ThrowIfFailed(DirectSoundCreate8(
		NULL,
		&sound_device,
		NULL));

	//Set the co-operative level "so the format of the primary sound buffer can be modified."
	ThrowIfFailed(sound_device->SetCooperativeLevel(hwnd, DSSCL_PRIORITY));

	/* Setup the primary buffer*/
	DSBUFFERDESC primary_sound_buffer_desc;
	primary_sound_buffer_desc.dwSize = sizeof(DSBUFFERDESC);
	primary_sound_buffer_desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
	primary_sound_buffer_desc.dwBufferBytes = 0;
	primary_sound_buffer_desc.dwReserved = 0;
	primary_sound_buffer_desc.lpwfxFormat = NULL;
	primary_sound_buffer_desc.guid3DAlgorithm = GUID_NULL;

	ThrowIfFailed(sound_device->CreateSoundBuffer(
		&primary_sound_buffer_desc,
		&primary_sound_buffer,
		NULL));
	 
	//setup the wave format of the primar_sound_buffer
	WAVEFORMATEX wave_format;
	wave_format.wFormatTag = WAVE_FORMAT_PCM;
	wave_format.nSamplesPerSec = 44100;
	wave_format.wBitsPerSample = 16;
	wave_format.nChannels = 2;
	wave_format.nBlockAlign = (wave_format.wBitsPerSample / 8) * wave_format.nChannels;
	wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
	wave_format.cbSize = 0;

	ThrowIfFailed(primary_sound_buffer->SetFormat(&wave_format));

	LoadSounds();
}

void DSAudioEngine::LoadSounds() {
	loadWavFileToBuffer((char*)"34012__erh__cinematic-deep-bass-rumble.wav", &ambience);
	loadWavFileToBuffer((char*)"158894__primeval-polypod__rocket-launch_01.wav", &rcs);
	loadWavFileToBuffer((char*)"318688__limitsnap-creations__rocket-thrust-effect.wav", &thruster);
	loadWavFileToBuffer((char*)"216385__rsilveira-88__alarm_01.wav", &alarm);
	PlaySpecificSound(ambience);
	ambience->Stop();
	PlaySpecificSound(rcs);
	rcs->Stop();
	PlaySpecificSound(thruster);
	thruster->Stop();
	PlaySpecificSound(alarm);
	alarm->Stop();
	
}

void DSAudioEngine::loadWavFileToBuffer(char* filename, IDirectSoundBuffer8** Buffer) {
	int error;
	FILE* filePtr;
	unsigned int count;
	WaveHeaderType waveFileHeader;
	WAVEFORMATEX waveFormat;
	DSBUFFERDESC bufferDesc;
	IDirectSoundBuffer* tempBuffer;
	unsigned char* waveData;
	unsigned char* bufferPtr;
	unsigned long bufferSize;

	// Open the wave file in binary.
	error = fopen_s(&filePtr, filename, "rb");
	
	// Read in the wave file header.
	count = fread(&waveFileHeader, sizeof(waveFileHeader), 1, filePtr);

	// Check that the chunk ID is the RIFF format.
	if ((waveFileHeader.chunkId[0] != 'R') || (waveFileHeader.chunkId[1] != 'I') ||
		(waveFileHeader.chunkId[2] != 'F') || (waveFileHeader.chunkId[3] != 'F')) {
		throw "loading .wav failed";
	}

	// Check that the file format is the WAVE format.
	if ((waveFileHeader.format[0] != 'W') || (waveFileHeader.format[1] != 'A') ||
		(waveFileHeader.format[2] != 'V') || (waveFileHeader.format[3] != 'E')) {
		throw "loading .wav failed";
	}

	// Check that the sub chunk ID is the fmt format.
	if ((waveFileHeader.subChunkId[0] != 'f') || (waveFileHeader.subChunkId[1] != 'm') ||
		(waveFileHeader.subChunkId[2] != 't') || (waveFileHeader.subChunkId[3] != ' ')) {
		throw "loading .wav failed";
	}

	// Check that the audio format is WAVE_FORMAT_PCM.
	if (waveFileHeader.audioFormat != WAVE_FORMAT_PCM) { throw "loading .wav failed"; }

	// Check that the wave file was recorded in stereo format.
	if (waveFileHeader.numChannels != 2) { throw "loading .wav failed"; }

	// Check that the wave file was recorded at a sample rate of 44.1 KHz.
	if (waveFileHeader.sampleRate != 44100) { throw "loading .wav failed"; }

	// Ensure that the wave file was recorded in 16 bit format.
	if (waveFileHeader.bitsPerSample != 16) { throw "loading .wav failed"; }

	// Check for the data chunk header.
	if ((waveFileHeader.dataChunkId[0] != 'd') || (waveFileHeader.dataChunkId[1] != 'a') ||
		(waveFileHeader.dataChunkId[2] != 't') || (waveFileHeader.dataChunkId[3] != 'a')){
		throw "loading .wav failed";
	}

	// Set the wave format of secondary buffer that this wave file will be loaded onto.
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nSamplesPerSec = 44100;
	waveFormat.wBitsPerSample = 16;
	waveFormat.nChannels = 2;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;

	// Set the buffer description of the secondary sound buffer that the wave file will be loaded onto.
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags = DSBCAPS_CTRLVOLUME;
	bufferDesc.dwBufferBytes = waveFileHeader.dataSize;
	bufferDesc.dwReserved = 0;
	bufferDesc.lpwfxFormat = &waveFormat;
	bufferDesc.guid3DAlgorithm = GUID_NULL;

	// Create a temporary sound buffer with the specific buffer settings.
	ThrowIfFailed(sound_device->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL));

	// Test the buffer format against the direct sound 8 interface and create the secondary buffer.
	ThrowIfFailed(tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)Buffer));

	// Release the temporary buffer.
	tempBuffer->Release();
	tempBuffer = 0;
	

	// Move to the beginning of the wave data which starts at the end of the data chunk header.
	fseek(filePtr, sizeof(WaveHeaderType), SEEK_SET);

	// Create a temporary buffer to hold the wave file data.
	waveData = new unsigned char[waveFileHeader.dataSize];
	if (!waveData) { throw "loading .wav failed"; }

	// Read in the wave file data into the newly created buffer.
	count = fread(waveData, 1, waveFileHeader.dataSize, filePtr);
	if (count != waveFileHeader.dataSize) { throw "loading .wav failed"; }

	// Close the file once done reading.
	error = fclose(filePtr);

	// Lock the secondary buffer to write wave data into it.
	ThrowIfFailed((*Buffer)->Lock(0, waveFileHeader.dataSize, (void**)&bufferPtr, (DWORD*)&bufferSize, NULL, 0, 0));

	// Copy the wave data into the buffer.
	memcpy(bufferPtr, waveData, waveFileHeader.dataSize);

	// Unlock the secondary buffer after the data has been written to it.
	ThrowIfFailed((*Buffer)->Unlock((void*)bufferPtr, bufferSize, NULL, 0));

	// Release the wave data since it was copied into the secondary buffer.
	delete[] waveData;
	waveData = 0;

}

void DSAudioEngine::CreateSoundBuffer(LPDIRECTSOUNDBUFFER8* Sound_Buffer) {
}

void DSAudioEngine::PlayWaveFile()
{
}

void DSAudioEngine::PlayWaveFile(char* Name) {

}

void DSAudioEngine::PlaySpecificSound(Sound SOUND) {
}

void DSAudioEngine::PlaySpecificSound(IDirectSoundBuffer8* BUFFER_PTR) {

	// Set position at the beginning of the sound buffer.
	ThrowIfFailed(BUFFER_PTR->SetCurrentPosition(0));

	// Set volume of the buffer to 100%.
	ThrowIfFailed(BUFFER_PTR->SetVolume(DSBVOLUME_MAX));

	// Play the contents of the secondary sound buffer.
	ThrowIfFailed(BUFFER_PTR->Play(0, 0, DSBPLAY_LOOPING));

}