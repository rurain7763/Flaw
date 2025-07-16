#include "pch.h"
#include "Prefab.h"
#include "Scene.h"
#include "Serialization.h"
#include "Log/Log.h"
#include "Scripting.h"
#include "MonoScriptSystem.h"

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
			_componentCreators[uuid] = [entityNode](const std::unordered_map<UUID, UUID>& remapedUUIDs, Entity& entity) {
				std::unordered_map<std::string, std::function<void(const YAML::iterator::value_type&, Entity&)>> userComponentDeserializer;
				userComponentDeserializer[TypeName<MonoScriptComponent>().data()] = [&remapedUUIDs](const YAML::iterator::value_type& component, Entity& entity) {
					std::string scriptName = component.second["Name"].as<std::string>();
					std::vector<MonoScriptComponent::FieldInfo> fields;

					auto fieldsNode = component.second["Fields"];
					for (const auto& field : fieldsNode) {
						MonoScriptComponent::FieldInfo fieldInfo;
						fieldInfo.fieldName = field.first.as<std::string>();
						fieldInfo.fieldType = field.second[0].as<std::string>();
						fieldInfo.fieldValue = field.second[1].as<std::string>();

						if (Scripting::HasMonoClass(fieldInfo.fieldType.c_str())) {
							auto fieldClass = Scripting::GetMonoClass(fieldInfo.fieldType.c_str());

							if (Scripting::IsMonoComponent(fieldClass) || fieldClass == Scripting::GetMonoClass(Scripting::MonoEntityClassName)) {
								fieldInfo.SetValue(remapedUUIDs.at(fieldInfo.As<UUID>()));
							}
						}

						fields.emplace_back(std::move(fieldInfo));
					}

					entity.AddComponent<MonoScriptComponent>(scriptName.data(), fields);
				};

				Deserialize(entityNode, entity, userComponentDeserializer);
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
		auto& monoScriptSys = scene.GetMonoScriptSystem();

		Entity root;

		std::unordered_map<UUID, UUID> remapedUUIDs;
		for (const auto& [uuid, creator] : _componentCreators) {
			bool isRoot = !_parentMap[uuid].IsValid();

			Entity entt = scene.CreateEntity();
			if (isRoot) {
				root = entt;
			}

			remapedUUIDs[uuid] = entt.GetUUID();
		}

		for (const auto& [childUUID, parentUUID] : _parentMap) {
			if (!parentUUID.IsValid()) {
				continue; // skip root entity
			}

			Entity childEntity = scene.FindEntityByUUID(remapedUUIDs[childUUID]);
			Entity parentEntity = scene.FindEntityByUUID(remapedUUIDs[parentUUID]);

			if (childEntity && parentEntity) {
				childEntity.SetParent(parentEntity);
			}
			else {
				Log::Error("Failed to set parent for entity with UUID: %lld", childUUID);
			}
		}

		monoScriptSys.BeginDeferredInitComponents();
		for (const auto& [uuid, creator] : _componentCreators) {
			const UUID& newUUID = remapedUUIDs.at(uuid);

			Entity entt = scene.FindEntityByUUID(newUUID);

			creator(remapedUUIDs, entt);
		}
		monoScriptSys.EndDeferredInitComponents();

		return root;
	}

	Entity Prefab::CreateEntity(Scene& scene, const vec3& position, const vec3& rotation, const vec3& scale) {
		Entity root = CreateEntity(scene);

		// NOTE: because creator set transform to default, we need to set it again
		auto& transComp = root.GetComponent<TransformComponent>();
		transComp.position = position;
		transComp.rotation = rotation;
		transComp.scale = scale;

		return root;
	}

	std::vector<int8_t> Prefab::ExportData(Entity entity) {
		std::unordered_map<UUID, UUID> remapedUUIDs;
		std::unordered_map<UUID, UUID> parentMap;

		UUID idCounter = 0;
		parentMap[idCounter] = UUID();
		remapedUUIDs[entity.GetUUID()] = idCounter++;

		std::queue<Entity> queue;
		queue.push(entity);
		while (!queue.empty()) {
			Entity current = queue.front();
			queue.pop();

			current.EachChildren([&queue, &remapedUUIDs, &parentMap, &idCounter](const Entity& child) {
				parentMap[idCounter] = remapedUUIDs[child.GetParent().GetUUID()];
				remapedUUIDs[child.GetUUID()] = idCounter++;

				queue.push(child);
			});
		}

		YAML::Emitter out;
		out << YAML::BeginMap;
		{
			out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
			{
				for (const auto& [oldUUID, newUUID] : remapedUUIDs) {
					Entity current = entity.GetScene().FindEntityByUUID(oldUUID);
					auto& enttComp = current.GetComponent<EntityComponent>();

					enttComp.uuid = newUUID;
					
					std::vector<MonoScriptComponent::FieldInfo> backupFields;
					if (current.HasComponent<MonoScriptComponent>()) {
						auto& monoComp = current.GetComponent<MonoScriptComponent>();

						backupFields = monoComp.fields;
						for (auto it = monoComp.fields.begin(); it != monoComp.fields.end();) {
							auto& fields = *it;

							if (Scripting::HasMonoClass(fields.fieldType.c_str())) {
								auto fieldClass = Scripting::GetMonoClass(fields.fieldType.c_str());

								if (Scripting::IsMonoComponent(fieldClass) || fieldClass == Scripting::GetMonoClass(Scripting::MonoEntityClassName)) {
									UUID currentUUID = fields.As<UUID>();

									auto uuidIt = remapedUUIDs.find(currentUUID);
									if (uuidIt == remapedUUIDs.end()) {
										it = monoComp.fields.erase(it);
										continue;
									}

									fields.SetValue(uuidIt->second);
								}
							}

							it++;
						}
					}
					
					Serialize(out, current);

					if (current.HasComponent<MonoScriptComponent>()) {
						auto& monoComp = entity.GetComponent<MonoScriptComponent>();
						monoComp.fields = backupFields; // restore original fields
					}

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