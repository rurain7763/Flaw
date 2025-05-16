#include "pch.h"
#include "Sounds.h"
#include "Sound/FMod/FModSoundsContext.h"

namespace flaw {
	static SoundsContext* context = nullptr;

	void Sounds::Init() {
		context = new FModSoundsContext();
	}

	void Sounds::Update() {
		context->Update();
	}

	void Sounds::Cleanup() {
		delete context;
	}

	Ref<SoundSource> Sounds::CreateSoundSourceFromMemory(const int8_t* memory, int64_t size) {
		return context->CreateSoundSourceFromMemory(memory, size);
	}

	Ref<SoundSource> Sounds::CreateSoundSourceFromFile(const char* filePath) {
		return context->CreateSoundSourceFromFile(filePath);
	}

	void Sounds::SetListener(const SoundListener& listener) {
		context->SetListener(listener);
	}

	void Sounds::StopAllSounds() {
		context->StopAllSounds();
	}

	void Sounds::PauseAllSounds() {
		context->PauseAllSounds();
	}

	void Sounds::ResumeAllSounds() {
		context->ResumeAllSounds();
	}
}