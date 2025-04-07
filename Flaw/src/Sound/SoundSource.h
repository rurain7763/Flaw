#pragma once

#include "Core.h"
#include "SoundChannel.h"

namespace flaw {
	class SoundSource {
	public:
		SoundSource() = default;
		virtual ~SoundSource() = default;

		virtual bool IsValid() const = 0;

		// loopCount = -1 means infinite loop, 0 means play once, 1 means play twice, etc.
		virtual Ref<SoundChannel> Play(int32_t loopCount = 0) = 0;

		virtual float GetDurationSec() const = 0;
	};
}