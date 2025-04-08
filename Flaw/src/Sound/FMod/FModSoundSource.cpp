#include "pch.h"
#include "FModSoundSource.h"
#include "FModSoundsContext.h"
#include "FModSoundChannel.h"
#include "Log/Log.h"

#include <FMod/fmod_errors.h>

namespace flaw {
	FModSoundSource::FModSoundSource(FMOD::System* system, const char* filePath)
		: _system(system)
	{
		if (system->createSound(filePath, FMOD_DEFAULT | FMOD_LOOP_NORMAL | FMOD_3D, nullptr, &_sound) != FMOD_OK) {
			Log::Error("Failed to create sound from file: {}", filePath);
			return;
		}
	}

	FModSoundSource::FModSoundSource(FMOD::System* system, const int8_t* memory, int64_t size)
		: _system(system)
	{
		FMOD_CREATESOUNDEXINFO exinfo = {};
		exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		exinfo.length = size;

		FMOD_RESULT result = system->createSound((const char*)memory, FMOD_OPENMEMORY | FMOD_LOOP_NORMAL | FMOD_3D, &exinfo, &_sound);
		if (result != FMOD_OK) {
			Log::Error("Failed to create sound from memory : %s", FMOD_ErrorString(result));
			return;
		}
	}
	
	FModSoundSource::~FModSoundSource() {
		_sound->release();
	}

	Ref<SoundChannel> FModSoundSource::Play(int32_t loopCount) {
		if (!_sound) {
			Log::Error("Sound is not initialized");
			return nullptr;
		}

		FMOD::Channel* channel;
		if (_system->playSound(_sound, nullptr, false, &channel) != FMOD_OK) {
			Log::Error("Failed to play sound");
			return nullptr;
		}

		channel->setLoopCount(loopCount);

		return CreateRef<FModSoundChannel>(channel);
	}

	float FModSoundSource::GetDurationSec() const {
		unsigned int length;
		_sound->getLength(&length, FMOD_TIMEUNIT_MS);
		return length / 1000.0f;
	}
}