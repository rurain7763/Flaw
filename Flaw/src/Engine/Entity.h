#pragma once

#include "Core.h"
#include "Scene.h"
#include "ECS/ECS.h"
#include "Utils/UUID.h"

namespace flaw {
	class Entity {
	public:
		Entity();
		Entity(entt::entity handle, Scene* scene);

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args) {
			FASSERT(!HasComponent<T>(), "Entity already has component!");
			return _scene->_registry.emplace<T>(_handle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent() const {
			FASSERT(HasComponent<T>(), "Entity does not have component!");
			return _scene->_registry.get<T>(_handle);
		}

		template<typename T>
		bool HasComponent() const {
			return _scene->_registry.any_of<T>(_handle);
		}

		template<typename T>
		void RemoveComponent() {
			FASSERT(HasComponent<T>(), "Entity does not have component!");
			_scene->_registry.remove<T>(_handle);
		}

		bool HasParent() const;
		void SetParent(Entity parent);
		void UnsetParent();
		Entity GetParent() const;

		bool HasChild() const;
		void EachChildren(const std::function<void(const Entity& entity)>& func) const;

		const UUID& GetUUID() const;
		const std::string& GetName() const;

		Scene& GetScene() const { return *_scene; }

		operator bool() const { return _handle != entt::null && _scene && _scene->_registry.valid(_handle); }
		operator uint32_t() const { return (uint32_t)_handle; }

		bool operator==(const Entity& other) const {
			return _handle == other._handle && _scene == other._scene;
		}

		bool operator!=(const Entity& other) const {
			return !(*this == other);
		}

	private:
		entt::entity _handle;
		Scene* _scene;
	};
}
