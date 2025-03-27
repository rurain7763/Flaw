#pragma once

#include "Core.h"
#include "Entity.h"

namespace flaw {
	struct Scriptable {
	public:
		virtual ~Scriptable() = default;

		template <typename T>
		T& GetComponent() {
			return _entity.GetComponent<T>();
		}

	protected:
		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate() {}

	private:
		friend class Scene;

		Entity _entity;
	};
}