#pragma once

#include <Flaw.h>

namespace flaw {
	class EditorCamera {
    public:
        EditorCamera();

        void OnUpdatePerspective(const vec2& moveDelta);
        void OnUpdateOrthographic(const vec2& moveDelta);
        void OnUpdate();

		void SetAspectRatio(float aspectRatio) { this->_aspectRatio = aspectRatio; }

		const vec3& GetPosition() const { return _position; }

		mat4 GetViewMatrix() const;
		mat4 GetProjectionMatrix() const;

		bool IsMoving() const { return _moving; }
		bool IsPerspective() const { return _perspective; }

    private:
        bool _perspective;

		// perspective
		float _fov;

		// orthographic
		float _zoomRate;
		float _orthoSize;
		float _zoomSpeed;

		float _aspectRatio;
		float _nearClip;
		float _farClip;

		vec3 _position;
		vec3 _rotation;

		bool _moving;
        vec2 _prevMousePos;
	};
}