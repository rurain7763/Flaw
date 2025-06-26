#include "pch.h"
#include "Animator.h"

namespace flaw {
	void AnimatorAnimation1D::GetAnimationMatrices(Ref<Skeleton> skeleton, float time, std::vector<mat4>& animationMatrices) {
		const float normalizedTime = glm::clamp(time / GetDuration(), 0.f, 1.f);
		skeleton->GetAnimationMatrices(_animation, normalizedTime, animationMatrices);
	}

	float AnimatorAnimation1D::GetDuration() const {
		return _animation->GetDurationSec();
	}

	void AnimatorAnimation2D::GetAnimationMatrices(Ref<Skeleton> skeleton, float time, std::vector<mat4>& animationMatrices) {
		const float normalizedTime = glm::clamp(time / GetDuration(), 0.f, 1.f);
		skeleton->GetBlendedAnimationMatrices(_animation0, _animation1, normalizedTime, _blendFactor, animationMatrices);
	}

	float AnimatorAnimation2D::GetDuration() const {
		return std::max(_animation0->GetDurationSec(), _animation1->GetDurationSec());
	}

	bool AnimatorTransition::CanTransition() const {
		return _condition && _condition();
	}

	void AnimatorTransition::GetAnimationMatrices(float time, std::vector<mat4>& outMatrices) {
		Ref<Skeleton> _skeleton = _animator.GetSkeleton();
		Ref<AnimatorState> fromState = _animator.GetAnimatorState(_fromStateIndex);
		Ref<AnimatorState> toState = _animator.GetAnimatorState(_toStateIndex);

		std::vector<mat4> fromMatrices;
		fromState->GetAnimation()->GetAnimationMatrices(_skeleton, 1.f, fromMatrices);

		std::vector<mat4> toMatrices;
		toState->GetAnimation()->GetAnimationMatrices(_skeleton, 0.f, toMatrices);

		const float normalizedTime = glm::clamp(time / _duration, 0.f, 1.f);

		outMatrices.resize(fromMatrices.size());
		for (size_t i = 0; i < fromMatrices.size(); ++i) {
			vec3 positionFrom, positionTo;
			quat rotationFrom, rotationTo;
			vec3 scaleFrom, scaleTo;

			ExtractModelMatrix(fromMatrices[i], positionFrom, rotationFrom, scaleFrom);
			ExtractModelMatrix(toMatrices[i], positionTo, rotationTo, scaleTo);

			vec3 finalPosition = glm::mix(positionFrom, positionTo, normalizedTime);
			quat finalRotation = normalize(slerp(rotationFrom, rotationTo, normalizedTime));
			vec3 finalScale = glm::mix(scaleFrom, scaleTo, normalizedTime);

			outMatrices[i] = ModelMatrix(finalPosition, finalRotation, finalScale);
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

		StructuredBuffer::Descriptor desc = {};
		desc.elmSize = sizeof(mat4);
		desc.count = animator._skeleton->GetBoneCount();
		desc.bindFlags = BindFlag::ShaderResource;
		desc.accessFlags = AccessFlag::Write;

		_animationMatricesSB = Graphics::CreateStructuredBuffer(desc);
	}

	void AnimatorRuntime::SetToDefaultState() {
		_currentStateIndex = _animator._defaultStateIndex;
		_currentTransitionIndex = -1;
		_currentTime = 0.0f;
	}

	void AnimatorRuntime::PlayState(int32_t stateIndex) {
		_currentStateIndex = stateIndex;
		_currentTransitionIndex = -1;
		_currentTime = 0.0f;
	}

	void AnimatorRuntime::Update(float deltaTime) {
		if (IsInTransition()) {
			UpdateTransition(deltaTime);
		}
		else {
			UpdateState(deltaTime);
		}
	}

	void AnimatorRuntime::UpdateTransition(float deltaTime) {
		if (_currentTransitionIndex == -1) {
			return;
		}

		Ref<AnimatorTransition> transition = _animator._transitions[_currentTransitionIndex];

		std::vector<mat4> transitionMatrices;
		transition->GetAnimationMatrices(_currentTime, transitionMatrices);

		_animationMatricesSB->Update(transitionMatrices.data(), transitionMatrices.size() * sizeof(mat4));

		_currentTime += deltaTime;
		if (_currentTime >= transition->GetDuration()) {
			_currentTransitionIndex = -1;
			_currentStateIndex = transition->GetToStateIndex();
			_currentTime = 0.0f;
		}
	}

	void AnimatorRuntime::UpdateState(float deltaTime) {
		if (_currentStateIndex == -1) {
			return;
		}

		Ref<AnimatorState> currentState = _animator._states[_currentStateIndex];

		std::vector<mat4> animationMatrices;
		currentState->GetAnimation()->GetAnimationMatrices(_animator._skeleton, _currentTime, animationMatrices);
		_animationMatricesSB->Update(animationMatrices.data(), animationMatrices.size() * sizeof(mat4));
		
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