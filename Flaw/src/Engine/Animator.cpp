#include "pch.h"
#include "Animator.h"
#include "Log/Log.h"

namespace flaw {
	void AnimatorAnimation1D::GetAnimationMatrices(Ref<Skeleton> skeleton, float time, std::vector<mat4>& animatedBoneMatrices, std::vector<mat4>& animatedSkinMatrices) {
		const float normalizedTime = glm::clamp(time / GetDuration(), 0.f, 1.f);
		skeleton->GetAnimatedBoneAndSkinMatrices(_animation, normalizedTime, animatedBoneMatrices, animatedSkinMatrices);
	}

	float AnimatorAnimation1D::GetDuration() const {
		return _animation->GetDurationSec();
	}

	void AnimatorAnimation2D::GetAnimationMatrices(Ref<Skeleton> skeleton, float time, std::vector<mat4>& animatedBoneMatrices, std::vector<mat4>& animatedSkinMatrices) {
		const float normalizedTime = glm::clamp(time / GetDuration(), 0.f, 1.f);
		skeleton->GetBlendedAnimatedBoneAndSkinMatrices(_animation0, _animation1, normalizedTime, _blendFactor, animatedBoneMatrices, animatedSkinMatrices);
	}

	float AnimatorAnimation2D::GetDuration() const {
		return std::max(_animation0->GetDurationSec(), _animation1->GetDurationSec());
	}

	bool AnimatorTransition::CanTransition() const {
		return _condition && _condition();
	}

	void AnimatorTransition::GetAnimationMatrices(float time, std::vector<mat4>& animatedBoneMatrices, std::vector<mat4>& animatedSkinMatrices) {
		Ref<Skeleton> _skeleton = _animator.GetSkeleton();
		Ref<AnimatorState> fromState = _animator.GetAnimatorState(_fromStateIndex);
		Ref<AnimatorState> toState = _animator.GetAnimatorState(_toStateIndex);

		std::vector<mat4> fromBoneMatrices;
		std::vector<mat4> fromSkinMatrices;
		fromState->GetAnimation()->GetAnimationMatrices(_skeleton, 1.f, fromBoneMatrices, fromSkinMatrices);

		std::vector<mat4> toBoneMatrices;
		std::vector<mat4> toSkinMatrices;
		toState->GetAnimation()->GetAnimationMatrices(_skeleton, 0.f, toBoneMatrices, toSkinMatrices);

		const float normalizedTime = glm::clamp(time / _duration, 0.f, 1.f);

		animatedBoneMatrices.resize(fromBoneMatrices.size());
		animatedSkinMatrices.resize(fromSkinMatrices.size());
		for (size_t i = 0; i < fromBoneMatrices.size(); ++i) {
			vec3 positionFrom, positionTo;
			quat rotationFrom, rotationTo;
			vec3 scaleFrom, scaleTo;

			ExtractModelMatrix(fromBoneMatrices[i], positionFrom, rotationFrom, scaleFrom);
			ExtractModelMatrix(toBoneMatrices[i], positionTo, rotationTo, scaleTo);

			vec3 finalPosition = glm::mix(positionFrom, positionTo, normalizedTime);
			quat finalRotation = normalize(slerp(rotationFrom, rotationTo, normalizedTime));
			vec3 finalScale = glm::mix(scaleFrom, scaleTo, normalizedTime);

			animatedBoneMatrices[i] = ModelMatrix(finalPosition, finalRotation, finalScale);

			ExtractModelMatrix(fromSkinMatrices[i], positionFrom, rotationFrom, scaleFrom);
			ExtractModelMatrix(toSkinMatrices[i], positionTo, rotationTo, scaleTo);

			finalPosition = glm::mix(positionFrom, positionTo, normalizedTime);
			finalRotation = normalize(slerp(rotationFrom, rotationTo, normalizedTime));
			finalScale = glm::mix(scaleFrom, scaleTo, normalizedTime);

			animatedSkinMatrices[i] = ModelMatrix(finalPosition, finalRotation, finalScale);
		}
	}
	
	Animator::Animator(const Animator::Descriptor& desc)
		: _skeleton(desc.skeleton)
		, _states(desc.states)
		, _transitions(desc.transitions)
		, _defaultStateIndex(desc.defaultStateIndex)
	{	
	}

	AnimatorRuntime::AnimatorRuntime(Animator& animator) 
		: _animator(animator)
		, _currentStateIndex(-1)
		, _currentTransitionIndex(-1)
		, _currentTime(0.0f)
	{
		_currentStateIndex = animator._defaultStateIndex;
	}

	void AnimatorRuntime::SetToDefaultState() {
		_currentStateIndex = _animator._defaultStateIndex;
		_currentTransitionIndex = -1;
		_currentTime = 0.0f;
	}

	void AnimatorRuntime::PlayState(int32_t stateIndex) {
		if (stateIndex < 0 || stateIndex >= _animator._states.size()) {
			Log::Error("AnimatorRuntime::PlayState: Invalid state index %d", stateIndex);
			return;
		}

		_currentStateIndex = stateIndex;
		_currentTransitionIndex = -1;
		_currentTime = 0.0f;
	}

	void AnimatorRuntime::Update(float deltaTime, std::vector<mat4>& animatedBoneMatrices, std::vector<mat4>& animatedSkinMatrices) {
		if (IsInTransition()) {
			UpdateTransition(deltaTime, animatedBoneMatrices, animatedSkinMatrices);
		}
		else {
			UpdateState(deltaTime, animatedBoneMatrices, animatedSkinMatrices);
		}
	}

	void AnimatorRuntime::UpdateTransition(float deltaTime, std::vector<mat4>& animatedBoneMatrices, std::vector<mat4>& animatedSkinMatrices) {
		if (_currentTransitionIndex == -1) {
			return;
		}

		Ref<AnimatorTransition> transition = _animator._transitions[_currentTransitionIndex];

		transition->GetAnimationMatrices(_currentTime, animatedBoneMatrices, animatedSkinMatrices);

		_currentTime += deltaTime;
		if (_currentTime >= transition->GetDuration()) {
			_currentTransitionIndex = -1;
			_currentStateIndex = transition->GetToStateIndex();
			_currentTime = 0.0f;
		}
	}

	void AnimatorRuntime::UpdateState(float deltaTime, std::vector<mat4>& animatedBoneMatrices, std::vector<mat4>& animatedSkinMatrices) {
		if (_currentStateIndex == -1) {
			return;
		}

		Ref<AnimatorState> currentState = _animator._states[_currentStateIndex];

		currentState->GetAnimation()->GetAnimationMatrices(_animator._skeleton, _currentTime, animatedBoneMatrices, animatedSkinMatrices);
		
		_currentTime += deltaTime;
		if (currentState->IsLooping() && _currentTime >= currentState->GetAnimation()->GetDuration()) {
			_currentTime = 0.0f;
		}

		if (!currentState->HasTransition()) {
			return;
		}

		Ref<AnimatorTransition> transition = _animator._transitions[currentState->GetTransitionIndex()];
		if (!transition->CanTransition()) {
			return;
		}

		_currentTransitionIndex = currentState->GetTransitionIndex();
		_currentStateIndex = -1;
		_currentTime = 0.0f;
	}
}