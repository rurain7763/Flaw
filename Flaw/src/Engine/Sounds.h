#pragma once

#include "Sound/SoundsContext.h"

namespace flaw {
	class Sounds {
	public:
		static void Initialize();
		static void Update();
		static void Cleanup();

		static Ref<SoundSource> CreateSoundSourceFromMemory(const int8_t* memory, int64_t size);
		static Ref<SoundSource> CreateSoundSourceFromFile(const char* filePath);

		static void SetListener(const SoundListener& listener);

		static void StopAllSounds();
		static void PauseAllSounds();
		static void ResumeAllSounds();
	};
}