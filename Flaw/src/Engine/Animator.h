#pragma once

#include "Core.h"
#include "Skeleton.h"

#include <unordered_map>

namespace flaw {
	class Animator;

	class AnimatorAnimation {
	public:
		virtual ~AnimatorAnimation() = default;

		virtual void GetAnimationMatrices(Ref<Skeleton> skeleton, float time, std::vector<mat4>& animatedBoneMatrices, std::vector<mat4>& animatedSkinMatrices) = 0;
		virtual float GetDuration() const = 0;
	};

	class AnimatorAnimation1D : public AnimatorAnimation {
	public:
		AnimatorAnimation1D(Ref<SkeletalAnimation> animation) : _animation(animation) {}
		virtual ~AnimatorAnimation1D() = default;

		void GetAnimationMatrices(Ref<Skeleton> skeleton, float time, std::vector<mat4>& animatedBoneMatrices, std::vector<mat4>& animatedSkinMatrices) override;
		float GetDuration() const override;

	private:
		Ref<SkeletalAnimation> _animation;
	};

	class AnimatorAnimation2D : public AnimatorAnimation {
	public:
		AnimatorAnimation2D(Ref<SkeletalAnimation> animation0, Ref<SkeletalAnimation> animation1) 
			: _animation0(animation0)
			, _animation1(animation1)
			, _blendFactor(0.5f) 
		{
		}

		virtual ~AnimatorAnimation2D() = default;

		void GetAnimationMatrices(Ref<Skeleton> skeleton, float time, std::vector<mat4>& animatedBoneMatrices, std::vector<mat4>& animatedSkinMatrices) override;
		float GetDuration() const override;

		void SetBlendFactor(float factor) { _blendFactor = glm::clamp(factor, 0.0f, 1.0f); }

	private:
		Ref<SkeletalAnimation> _animation0, _animation1;

		float _blendFactor;
	};

	class AnimatorTransition {
	public:
		AnimatorTransition(Animator& animator) : _animator(animator) {}

		bool CanTransition() const;

		void GetAnimationMatrices(float time, std::vector<mat4>& animatedBoneMatrices, std::vector<mat4>& animatedSkinMatrices);

		float GetDuration() const { return _duration; }

		int32_t GetFromStateIndex() const { return _fromStateIndex; }
		int32_t GetToStateIndex() const { return _toStateIndex; }

	private:
		Animator& _animator;

		std::function<bool()> _condition;
		int32_t _fromStateIndex = -1;
		int32_t _toStateIndex = -1;

		float _duration = 0.0f;
	};

	class AnimatorState {
	public:
		void SetAnimation(Ref<AnimatorAnimation> animation) { _animation = animation; }
		Ref<AnimatorAnimation> GetAnimation() const { return _animation; }

		void SetLoop(bool loop) { _loop = loop; }
		bool IsLooping() const { return _loop; }

		bool HasTransition() const { return _transitionIndex != -1; }
		int32_t GetTransitionIndex() const { return _transitionIndex; }

	private:
		Ref<AnimatorAnimation> _animation;
		bool _loop = false;
		int32_t _transitionIndex = -1;
	};

	class Animator {
	public:
		struct Descriptor {
			Ref<Skeleton> skeleton;
			std::vector<Ref<AnimatorState>> states;
			std::vector<Ref<AnimatorTransition>> transitions;
			int32_t defaultStateIndex = 0;
		};

		Animator(const Descriptor& desc);

		void SetSkeleton(Ref<Skeleton> skeleton) { _skeleton = skeleton; }
		Ref<Skeleton> GetSkeleton() const { return _skeleton; }

		Ref<AnimatorState> GetAnimatorState(int32_t index) const { return _states[index]; }

	private:
		friend class AnimatorRuntime;

		Ref<Skeleton> _skeleton;

		std::vector<Ref<AnimatorState>> _states;
		std::vector<Ref<AnimatorTransition>> _transitions;

		int32_t _defaultStateIndex;
	};

	class AnimatorRuntime {
	public:
		AnimatorRuntime() = default;
		AnimatorRuntime(Animator& animator);

		void SetToDefaultState();
		void PlayState(int32_t stateIndex);

		void Update(float deltaTime, std::vector<mat4>& animatedBoneMatrices, std::vector<mat4>& animatedSkinMatrices);

		bool IsInTransition() const { return _currentTransitionIndex != -1; }

	private:
		void UpdateTransition(float deltaTime, std::vector<mat4>& animatedBoneMatrices, std::vector<mat4>& animatedSkinMatrices);
		void UpdateState(float deltaTime, std::vector<mat4>& animatedBoneMatrices, std::vector<mat4>& animatedSkinMatrices);

	private:
		Animator& _animator;

		int32_t _currentStateIndex;
		int32_t _currentTransitionIndex;

		float _currentTime;
	};
}