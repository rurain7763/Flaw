#pragma once

#include "Core.h"

namespace flaw {
	class SoundChannel {
	public:
		SoundChannel() = default;
		virtual ~SoundChannel() = default;

		virtual void Stop() = 0;
		virtual void Pause() = 0;
		virtual void Resume() = 0;

		virtual float GetVolume() const = 0;
		virtual void SetVolume(float volume) = 0;

		virtual float GetPositionSec() const = 0;
		virtual void SetPositionSec(float sec) = 0;

		virtual bool IsPlaying() const = 0;
	};
}