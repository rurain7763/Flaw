#pragma once

#include "Core.h"

#include "SoundSource.h"
#include "SoundChannel.h"

namespace flaw {
	class SoundsContext {
	public:
		SoundsContext() = default;
		virtual ~SoundsContext() = default;

		virtual void Update() = 0;

		virtual Ref<SoundSource> CreateSoundSourceFromMemory(const int8_t* memory, int64_t size) = 0;
		virtual Ref<SoundSource> CreateSoundSourceFromFile(const char* filePath) = 0;

		virtual uint32_t GetChannelCount() const = 0;
	};
}