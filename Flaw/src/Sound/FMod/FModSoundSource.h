#pragma once

#include "Sound/SoundSource.h"

#include <FMod/fmod.hpp>

namespace flaw {
	class FModSoundsContext;

	class FModSoundSource : public SoundSource {
	public:
		FModSoundSource(FMOD::System* system, const char* filePath);
		FModSoundSource(FMOD::System* system, const int8_t* memory, int64_t size);
		~FModSoundSource();

		bool IsValid() const override { return _sound != nullptr; }
		Ref<SoundChannel> Play(int32_t loopCount = 0) override;
		float GetDurationSec() const override;

	private:
		FMOD::System* _system;
		FMOD::Sound* _sound;
	};
}