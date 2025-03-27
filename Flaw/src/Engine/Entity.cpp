#include "pch.h"
#include "Entity.h"
#include "Components.h"

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
}