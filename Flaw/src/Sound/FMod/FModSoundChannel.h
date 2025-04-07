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

		virtual float GetVolume() const override;
		virtual void SetVolume(float volume) override;

		virtual float GetPositionSec() const override;
		virtual void SetPositionSec(float sec) override;

		virtual bool IsPlaying() const override;

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