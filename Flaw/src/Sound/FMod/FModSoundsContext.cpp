#include "pch.h"
#include "FModSoundsContext.h"

#include "FModSoundSource.h"
#include "FModSoundChannel.h"

namespace flaw {
	FModSoundsContext::FModSoundsContext()
		: _system(nullptr)
		, _channels(32)
	{

		FMOD::System_Create(&_system);
		_system->init(_channels, FMOD_INIT_NORMAL, nullptr);
	}

	FModSoundsContext::~FModSoundsContext() {
		_system->release();
	}

	void FModSoundsContext::Update() {
		_system->update();
	}

	Ref<SoundSource> FModSoundsContext::CreateSoundSourceFromMemory(const int8_t* memory, int64_t size) {
		return CreateRef<FModSoundSource>(_system, memory, size);
	}

	Ref<SoundSource> FModSoundsContext::CreateSoundSourceFromFile(const char* filePath) {
		return CreateRef<FModSoundSource>(_system, filePath);
	}

	uint32_t FModSoundsContext::GetChannelCount() const {
		return _channels;
	}
}