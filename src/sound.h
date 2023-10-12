#pragma once
#include "schnitzel_lib.h"

// #############################################################################
//                           Sound Constants
// #############################################################################
static constexpr int MAX_CONCURRENT_SOUNDS = 16;
static constexpr int SOUNDS_BUFFER_SIZE = MB(128);
static constexpr int MAX_SOUND_PATH_LENGTH = 256;

static constexpr float FADE_DURATION = 1.0f;

// #############################################################################
//                           Sound Structs
// #############################################################################
enum SoundOptionBits
{
	SOUND_OPTION_FADE_OUT = BIT(0),
	SOUND_OPTION_FADE_IN = BIT(1),
	SOUND_OPTION_START = BIT(2),
	SOUND_OPTION_LOOP = BIT(3)
};
typedef int SoundOptions;

struct Sound
{
	char file[MAX_SOUND_PATH_LENGTH];
	SoundOptions options;
	int size;
	char* data;
};

struct SoundState
{
	// Buffer containing all Sounds
	int bytesUsed;
	char* allocatedsoundsBuffer;

	BumpAllocator* transientStorage;

	// Allocted sounds
	Array<Sound, MAX_CONCURRENT_SOUNDS> allocatedSounds;

	// Used by the platform to determine when to start and stop sounds
	Array<Sound, MAX_CONCURRENT_SOUNDS> playingSounds;
};

// #############################################################################
//                           Sound Globals
// #############################################################################
static SoundState* soundState;

// #############################################################################
//                           Sound Functions
// #############################################################################
void play_sound(char* soundName, SoundOptions options = 0)
{
	SM_ASSERT(soundName, "No Sound name supplied!");

	// We can stop sounds using this function but if no
	// options are supplied, at least SOUND_OPTION_START
	// should be used.
	options = options? options : SOUND_OPTION_START;
	if(!(options & SOUND_OPTION_START) &&
		 !(options & SOUND_OPTION_FADE_IN) &&
		 !(options & SOUND_OPTION_FADE_OUT))
	{
		options |= SOUND_OPTION_START;
	}

	Sound sound = {};
	sound.options = options;
	sprintf(sound.file, "assets/sounds/%s.wav", soundName);

	// Look for existing Sound to play
	for(int soundIdx = 0; soundIdx < soundState->allocatedSounds.count; soundIdx++)
	{
		Sound allocatedSound = soundState->allocatedSounds[soundIdx];

		if(strcmp(allocatedSound.file, sound.file) == 0)
		{
			// Use allocated Sound
			allocatedSound.options = sound.options;
			soundState->playingSounds.add(allocatedSound);
			return;
		}
	}

	// Couldn't find a Sound, Load WAV file if presend and allocate
	WAVFile* wavFile = load_wav(sound.file, soundState->transientStorage);
	if(wavFile)
	{
		if(wavFile->header.dataChunkSize > SOUNDS_BUFFER_SIZE - soundState->bytesUsed)
		{
			SM_ASSERT(0, "Exausted Sounds Buffer!\nCapacity:\t%d\nBytes Used:\t%d\nSound Path:\t%s\nSound Size:\t%d",
									 SOUNDS_BUFFER_SIZE, soundState->bytesUsed, sound.file, wavFile->header.dataChunkSize);
			return;
		}
		sound.size = wavFile->header.dataChunkSize;
		sound.data = &soundState->allocatedsoundsBuffer[soundState->bytesUsed];
		soundState->bytesUsed += sound.size;
		memcpy(sound.data, &wavFile->dataBegin, sound.size);

		soundState->allocatedSounds.add(sound);
		soundState->playingSounds.add(sound);
	}
}

void stop_sound(char* soundName)
{
	play_sound(soundName, SOUND_OPTION_FADE_OUT);
}
