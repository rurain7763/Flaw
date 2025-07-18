#pragma once

#include <Flaw.h>

namespace flaw {
	class EditorCamera {
    public:
        EditorCamera();

        void OnUpdatePerspective(const vec2& moveDelta);
        void OnUpdateOrthographic(const vec2& moveDelta);
        void OnUpdate();

		Ref<Camera> GetCurrentCamera() const;

		void SetAspectRatio(float aspectRatio);

		mat4 GetViewMatrix() const;
		mat4 GetProjectionMatrix() const;

		const vec3& GetPosition() const { return _position; }
		vec3 GetFront() const { return QRotate(_rotation, Forward); }

		bool IsMoving() const { return _moving; }
		bool IsPerspective() const { return _perspective; }

    private:
        bool _perspective;

		Ref<PerspectiveCamera> _perspectiveCamera;
		Ref<OrthographicCamera> _orthographicCamera;

		// orthographic
		float _zoomRate;
		float _zoomSpeed;

		vec3 _position;
		vec3 _rotation;

		bool _moving;
        vec2 _prevMousePos;
	};
}