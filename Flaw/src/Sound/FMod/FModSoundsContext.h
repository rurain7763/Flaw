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

		virtual void SetListener(const SoundListener& listener) override;

		virtual void StopAllSounds() override;
		virtual void PauseAllSounds() override;
		virtual void ResumeAllSounds() override;

		virtual uint32_t GetChannelCount() const override;

	private:
		FMOD::System* _system;

		int32_t _channels;
	};
}