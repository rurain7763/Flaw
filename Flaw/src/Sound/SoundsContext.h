#pragma once

#include "Core.h"

#include "SoundSource.h"
#include "SoundChannel.h"
#include "Math/Math.h"

namespace flaw {
	struct SoundListener {
		vec3 position;
		vec3 velocity;
		vec3 forward;
		vec3 up;
	};

	class SoundsContext {
	public:
		SoundsContext() = default;
		virtual ~SoundsContext() = default;

		virtual void Update() = 0;

		virtual void SetListener(const SoundListener& listener) = 0;

		virtual void StopAllSounds() = 0;
		virtual void PauseAllSounds() = 0;
		virtual void ResumeAllSounds() = 0;

		virtual Ref<SoundSource> CreateSoundSourceFromMemory(const int8_t* memory, int64_t size) = 0;
		virtual Ref<SoundSource> CreateSoundSourceFromFile(const char* filePath) = 0;

		virtual uint32_t GetChannelCount() const = 0;
	};
}