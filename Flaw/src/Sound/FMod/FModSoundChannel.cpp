#include "pch.h"
#include "FModSoundChannel.h"

namespace flaw {
	FModSoundChannel::FModSoundChannel(FMOD::Channel* channel)
		: _channel(channel)
	{
		_channel->setUserData(this);
		_channel->setCallback(FModSoundChannel::ChannelCallback);
	}

	FModSoundChannel::~FModSoundChannel() {
		_channel->stop();
	}

	void FModSoundChannel::Stop() {
		_channel->stop();
	}

	void FModSoundChannel::Pause() {
		_channel->setPaused(true);
	}

	void FModSoundChannel::Resume() {
		_channel->setPaused(false);
	}

	void FModSoundChannel::SetPosition3D(const vec3& position) {
		FMOD_VECTOR pos = { position.x, position.y, position.z };
		_channel->set3DAttributes(&pos, nullptr);
	}

	void FModSoundChannel::SetVelocity3D(const vec3& velocity) {
		FMOD_VECTOR vel = { velocity.x, velocity.y, velocity.z };
		_channel->set3DAttributes(nullptr, &vel);
	}

	void FModSoundChannel::SetPositionAndVelocity3D(const vec3& position, const vec3& velocity) {
		FMOD_VECTOR pos = { position.x, position.y, position.z };
		FMOD_VECTOR vel = { velocity.x, velocity.y, velocity.z };
		_channel->set3DAttributes(&pos, &vel);
	}

	void FModSoundChannel::SetMute(bool mute) {
		_channel->setMute(mute);
	}

	void FModSoundChannel::SetLoop(int loopCount) {
		_channel->setLoopCount(loopCount);
	}

	float FModSoundChannel::GetVolume() const {
		float volume;
		_channel->getVolume(&volume);
		return volume;
	}

	void FModSoundChannel::SetVolume(float volume) {
		_channel->setVolume(volume);
	}

	float FModSoundChannel::GetPositionSec() const {
		unsigned int position;
		_channel->getPosition(&position, FMOD_TIMEUNIT_MS);
		return position / 1000.0f;
	}

	void FModSoundChannel::SetPositionSec(float sec) {
		_channel->setPosition(sec * 1000, FMOD_TIMEUNIT_MS);
	}

	bool FModSoundChannel::IsPlaying() const {
		bool playing;
		_channel->isPlaying(&playing);

		return playing;
	}

	FMOD_RESULT FModSoundChannel::ChannelCallback(
		FMOD_CHANNELCONTROL* channelcontrol,
		FMOD_CHANNELCONTROL_TYPE controltype,
		FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
		void* commanddata1,
		void* commanddata2)
	{
		FMOD::Channel* channel = (FMOD::Channel*)channelcontrol;

		FModSoundChannel* soundChannel = nullptr;
		channel->getUserData((void**)&soundChannel);

		if (soundChannel) {
			if (callbacktype == FMOD_CHANNELCONTROL_CALLBACK_END) {
				// 사운드 재생이 끝났을 때
			}
		}

		return FMOD_OK;
	}
}