#include "EditorCamera.h"

namespace flaw {
	EditorCamera::EditorCamera() 
		: _perspective(true)
		, _fov(45.0f)
		, _zoomRate(1.0f)
		, _orthoSize(1.0f)
		, _zoomSpeed(0.1f)
		, _aspectRatio(16.0f / 9.0f)
		, _nearClip(0.1f)
		, _farClip(1000.0f)
		, _position(0.0f, 0.0f, -5.0f)
		, _rotation(0.0f)
    {
        _prevMousePos = vec2(Input::GetMouseX(), Input::GetMouseY());
	}

    void EditorCamera::OnUpdatePerspective(const vec2& moveDelta) {
		// TODO: 이동 속도를 조절할 수 있도록 해야함
        const float speed = 10.0f;

        if (length2(moveDelta) > 0.0f) {
            vec3 forward = QRotate(_rotation, Forward);
            vec3 right = glm::cross(Up, forward);
            vec2 normalized = normalize(moveDelta);

            _position += vec3(right.x, right.y, right.z) * normalized.x * speed * Time::DeltaTime();
            _position += vec3(forward.x, forward.y, forward.z) * normalized.y * speed * Time::DeltaTime();
        }

        vec2 mousePos = vec2(Input::GetMouseX(), Input::GetMouseY());
        vec2 mouseDelta = mousePos - _prevMousePos;

        if (!EpsilonEqual(glm::length2(mouseDelta), 0.f)) {
            mouseDelta = glm::normalize(mouseDelta);

            _rotation.y += mouseDelta.x * glm::pi<float>() * Time::DeltaTime();
            _rotation.x += mouseDelta.y * glm::pi<float>() * Time::DeltaTime();

            glm::clamp(_rotation.x, -glm::half_pi<float>(), glm::half_pi<float>());
        }

        _prevMousePos = mousePos;
    }

    void EditorCamera::OnUpdateOrthographic(const vec2& moveDelta) {
        // TODO: 이동 속도를 조절할 수 있도록 해야함
        const float speed = 10.0f;

        if (length2(moveDelta) > 0.0f) {
            vec2 normalized = normalize(moveDelta);
            _position += vec3(normalized.x, normalized.y, 0.0f) * speed * Time::DeltaTime();
        }

        const float ms = Input::GetMouseScrollY();
        if (ms != 0.0) {
            _zoomRate += ms * _zoomSpeed;
            if (_zoomRate < 0.1f) {
                _zoomRate = 0.1f;
            }
        }
    }

    void EditorCamera::OnUpdate() {
        if (!Input::GetMouseButton(MouseButton::Right)) {
			_moving = false;
            return;
        }

        vec2 delta = vec2(0.0f);

        if (Input::GetKey(KeyCode::W)) {
            delta.y = 1.0f;
        }
        else if (Input::GetKey(KeyCode::S)) {
            delta.y = -1.0f;
        }

        if (Input::GetKey(KeyCode::A)) {
            delta.x = -1.0f;
        }
        else if (Input::GetKey(KeyCode::D)) {
            delta.x = 1.0f;
        }

        if (_perspective) {
            OnUpdatePerspective(delta);
        }
        else {
            OnUpdateOrthographic(delta);
        }

        _moving = !EpsilonEqual(glm::length2(delta), 0);
    };

    mat4 EditorCamera::GetWorldMatrix() const {
        return ModelMatrix(_position, _rotation, vec3(1.0f));
    }

    mat4 EditorCamera::GetViewMatrix() const {
        return ViewMatrix(_position, _rotation);
    }

	mat4 EditorCamera::GetProjectionMatrix() const {
		if (_perspective) {
			return Perspective(_fov, _aspectRatio, _nearClip, _farClip);
		}
		else {
            const float height = _orthoSize * _zoomRate;
            const float width = height * _aspectRatio;
			return Orthographic(-width, width, -height, height, _nearClip, _farClip);
		}
	}
}