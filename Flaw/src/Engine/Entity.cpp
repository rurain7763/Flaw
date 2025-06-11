#include "pch.h"
#include "Entity.h"
#include "Components.h"
#include "Log/Log.h"

namespace flaw {
	Entity::Entity() 
		: _handle(entt::null)
		, _scene(nullptr)
	{
	}

	Entity::Entity(entt::entity handle, Scene* scene)
		: _handle(handle)
		, _scene(scene)
	{
	}

	const UUID& Entity::GetUUID() const {
		return _scene->_registry.get<EntityComponent>(_handle).uuid;
	}

	const std::string& Entity::GetName() const {
		return _scene->_registry.get<EntityComponent>(_handle).name;
	}

	bool Entity::HasParent() const {
		return _scene->_parentMap.find(GetUUID()) != _scene->_parentMap.end();
	}

	void Entity::SetParent(Entity parent) {
		UUID thisUUID = GetUUID();
		UUID parentUUID = parent.GetUUID();

		if (thisUUID == parentUUID) {
			// can't set self as parent
			return;
		}

		// check if set parent to already parent
		auto oldParentIt = _scene->_parentMap.find(thisUUID);
		if (oldParentIt != _scene->_parentMap.end() && oldParentIt->second == parentUUID) {
			return;
		}

		// check if parent is a child of this object
		std::queue<UUID> q;
		q.push(parentUUID);
		while (!q.empty()) {
			auto currentUUID = q.front();
			q.pop();

			if (currentUUID == thisUUID) {
				// parent is a child of this object, so we can't set it as parent
				return;
			}

			auto parentIt = _scene->_parentMap.find(currentUUID);
			if (parentIt != _scene->_parentMap.end()) {
				q.push(parentIt->second);
			}
		}

		// remove this object from its old parent
		if (oldParentIt != _scene->_parentMap.end()) {
			_scene->_childMap[oldParentIt->second].erase(thisUUID);
		}

		// excute
		_scene->_parentMap[thisUUID] = parentUUID;
		_scene->_childMap[parentUUID].insert(thisUUID);

		GetComponent<TransformComponent>().dirty = true;
	}

	void Entity::UnsetParent() {
		UUID thisUUID = GetUUID();

		auto parentIt = _scene->_parentMap.find(thisUUID);
		if (parentIt == _scene->_parentMap.end()) {
			// no parent to unset
			return;
		}

		// remove this object from its parent
		_scene->_childMap[parentIt->second].erase(thisUUID);
		_scene->_parentMap.erase(thisUUID);

		GetComponent<TransformComponent>().dirty = true;
	}

	Entity Entity::GetParent() const {
		auto it = _scene->_parentMap.find(GetUUID());
		if (it != _scene->_parentMap.end()) {
			return _scene->FindEntityByUUID(it->second);
		}
		return Entity();
	}

	bool Entity::HasChild() const {
		auto it = _scene->_childMap.find(GetUUID());
		return it != _scene->_childMap.end() && !it->second.empty();
	}

	void Entity::EachChildren(const std::function<void(const Entity& entity)>& func) const {
		auto it = _scene->_childMap.find(GetUUID());
		if (it != _scene->_childMap.end()) {
			for (const auto& childUUID : it->second) {
				func(_scene->FindEntityByUUID(childUUID));
			}
		}
	}
}