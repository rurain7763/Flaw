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

	void FModSoundsContext::SetListener(const SoundListener& listener) {
		FMOD_VECTOR position = { listener.position.x, listener.position.y, listener.position.z };
		FMOD_VECTOR velocity = { listener.velocity.x, listener.velocity.y, listener.velocity.z };
		FMOD_VECTOR forward = { listener.forward.x, listener.forward.y, listener.forward.z };
		FMOD_VECTOR up = { listener.up.x, listener.up.y, listener.up.z };
		_system->set3DListenerAttributes(0, &position, &velocity, &forward, &up);
	}

	void FModSoundsContext::StopAllSounds() {
		FMOD::ChannelGroup* masterGroup;
		_system->getMasterChannelGroup(&masterGroup);

		masterGroup->stop();
	}

	void FModSoundsContext::PauseAllSounds() {
		FMOD::ChannelGroup* masterGroup;
		_system->getMasterChannelGroup(&masterGroup);

		masterGroup->setPaused(true);
	}

	void FModSoundsContext::ResumeAllSounds() {
		FMOD::ChannelGroup* masterGroup;
		_system->getMasterChannelGroup(&masterGroup);

		masterGroup->setPaused(false);
	}

	uint32_t FModSoundsContext::GetChannelCount() const {
		return _channels;
	}
}