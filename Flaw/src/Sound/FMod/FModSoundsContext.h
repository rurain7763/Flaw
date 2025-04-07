#pragma once

#include "Sound/SoundsContext.h"

#include <FMod/fmod.hpp>

namespace flaw {
	class FModSoundsContext : public SoundsContext {
	public:
		FModSoundsContext();
		~FModSoundsContext();

		virtual void Update() override;

		virtual Ref<SoundSource> CreateSoundSourceFromMemory(const int8_t* memory, int64_t size) override;
		virtual Ref<SoundSource> CreateSoundSourceFromFile(const char* filePath) override;

		virtual uint32_t GetChannelCount() const override;

	private:
		FMOD::System* _system;

		int32_t _channels;
	};
}