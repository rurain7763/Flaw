#pragma once

#include "Core.h"
#include "Math/Math.h"

namespace flaw {
	class SoundChannel {
	public:
		SoundChannel() = default;
		virtual ~SoundChannel() = default;

		virtual void Stop() = 0;
		virtual void Pause() = 0;
		virtual void Resume() = 0;

		virtual void SetPosition3D(const vec3& position) = 0;
		virtual void SetVelocity3D(const vec3& velocity) = 0;
		virtual void SetPositionAndVelocity3D(const vec3& position, const vec3& velocity) = 0;

		virtual void SetLoop(int loopCount) = 0;

		virtual void SetMute(bool mute) = 0;

		virtual float GetVolume() const = 0;
		virtual void SetVolume(float volume) = 0;

		virtual float GetPositionSec() const = 0;
		virtual void SetPositionSec(float sec) = 0;

		virtual bool IsPlaying() const = 0;
	};
}