#include "pch.h"
#include "Prefab.h"
#include "Scene.h"
#include "Serialization.h"
#include "Components.h"
#include "Log/Log.h"

namespace flaw {
	Prefab::Prefab(const int8_t* data) {
		YAML::Node node = YAML::Load(reinterpret_cast<const char*>(data));
		if (!node) {
			Log::Error("Failed to load prefab data");
			return;
		}

		auto entities = node["Entities"];
		for (const auto& entityNode : entities) {
			UUID uuid = entityNode["Entity"].as<UUID>();
			_componentCreators[uuid] = [entityNode](Entity& entity) {
				Deserialize(entityNode, entity);
			};
		}

		auto parentMapNode = node["ParentMap"];
		for (const auto& parent : parentMapNode) {
			UUID childUUID = parent.first.as<UUID>();
			UUID parentUUID = parent.second.as<UUID>();
			_parentMap[childUUID] = parentUUID;
		}
	}

	Entity Prefab::CreateEntity(Scene& scene) {
		Entity root;

		std::unordered_map<UUID, UUID> newUUIDs;
		for (const auto& [uuid, creator] : _componentCreators) {
			Entity entt = scene.CreateEntity();
			if (!_parentMap[uuid].IsValid()) {
				root = entt;
			}

			creator(entt);

			newUUIDs[uuid] = entt.GetUUID();
		}

		for (const auto& [childUUID, parentUUID] : _parentMap) {
			if (!parentUUID.IsValid()) {
				continue; // skip root entity
			}

			Entity childEntity = scene.FindEntityByUUID(newUUIDs[childUUID]);
			Entity parentEntity = scene.FindEntityByUUID(newUUIDs[parentUUID]);

			if (childEntity && parentEntity) {
				childEntity.SetParent(parentEntity);
			}
			else {
				Log::Error("Failed to set parent for entity with UUID: %s", childUUID);
			}
		}

		return root;
	}

	std::vector<int8_t> Prefab::ExportData(Entity entity) {
		std::unordered_map<UUID, UUID> newUUIDs;
		std::unordered_map<UUID, UUID> parentMap;

		UUID idCounter = 0;
		parentMap[idCounter] = UUID();
		newUUIDs[entity.GetUUID()] = idCounter++;

		std::queue<Entity> queue;
		queue.push(entity);
		while (!queue.empty()) {
			Entity current = queue.front();
			queue.pop();

			current.EachChildren([&queue, &newUUIDs, &parentMap, &idCounter](const Entity& child) {
				parentMap[idCounter] = newUUIDs[child.GetParent().GetUUID()];
				newUUIDs[child.GetUUID()] = idCounter++;

				queue.push(child);
			});
		}

		YAML::Emitter out;
		out << YAML::BeginMap;
		{
			out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
			{
				for (const auto& [oldUUID, newUUID] : newUUIDs) {
					Entity current = entity.GetScene().FindEntityByUUID(oldUUID);
					auto& enttComp = current.GetComponent<EntityComponent>();

					enttComp.uuid = newUUID;
					Serialize(out, current);
					enttComp.uuid = oldUUID; // restore original UUID
				}
			}
			out << YAML::EndSeq;

			out << YAML::Key << "ParentMap" << YAML::Value << YAML::BeginMap;
			{
				for (const auto& [childUUID, parentUUID] : parentMap) {
					out << YAML::Key << childUUID << YAML::Value << parentUUID;
				}
			}
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		return std::vector<int8_t>(out.c_str(), out.c_str() + out.size());
	}
}