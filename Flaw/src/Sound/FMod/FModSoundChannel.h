#pragma once

#include "Sound/SoundChannel.h"

#include <FMod/fmod.hpp>

namespace flaw {
	class FModSoundChannel : public SoundChannel {
	public:
		FModSoundChannel(FMOD::Channel* channel);
		~FModSoundChannel();

		void Stop() override;
		void Pause() override;
		void Resume() override;

		void SetPosition3D(const vec3& position) override;
		void SetVelocity3D(const vec3& velocity) override;
		void SetPositionAndVelocity3D(const vec3& position, const vec3& velocity) override;

		void SetMute(bool mute) override;

		void SetLoop(int loopCount) override;

		float GetVolume() const override;
		void SetVolume(float volume) override;

		float GetPositionSec() const override;
		void SetPositionSec(float sec) override;

		bool IsPlaying() const override;

	private:
		static FMOD_RESULT ChannelCallback(
			FMOD_CHANNELCONTROL* channelcontrol, 
			FMOD_CHANNELCONTROL_TYPE controltype, 
			FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype, 
			void* commanddata1, 
			void* commanddata2
		);

	private:
		FMOD::Channel* _channel;
	};
}