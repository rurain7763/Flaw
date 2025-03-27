#include "pch.h"
#include "Serialization.h"
#include "Components.h"
#include "ECS/ECS.h"

#include "Project.h"
#include "Scene.h"
#include "Entity.h"

namespace flaw {
	void Serialize(YAML::Emitter& out, ProjectConfig& config) {
		out << YAML::BeginMap;
		{
			out << YAML::Key << "ProjectConfig" << YAML::Value;
			out << YAML::BeginMap;
			{
				out << YAML::Key << "Name" << YAML::Value << config.name;
				out << YAML::Key << "Path" << YAML::Value << config.path;
				out << YAML::Key << "StartScene" << YAML::Value << config.startScene;
			}
			out << YAML::EndMap;
		}
		out << YAML::EndMap;
	}

	void Serialize(YAML::Emitter& out, Entity& entity) {
		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

		out << YAML::Key << "Components";
		out << YAML::Value << YAML::BeginMap;

		if (entity.HasComponent<flaw::EntityComponent>()) {
			auto& comp = entity.GetComponent<flaw::EntityComponent>();
			out << YAML::Key << "EntityComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "UUID" << YAML::Value << comp.uuid;
			out << YAML::Key << "Name" << YAML::Value << comp.name;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::TransformComponent>()) {
			auto& comp = entity.GetComponent<flaw::TransformComponent>();
			out << YAML::Key << "TransformComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Position" << YAML::Value << comp.position;
			out << YAML::Key << "Rotation" << YAML::Value << comp.rotation;
			out << YAML::Key << "Scale" << YAML::Value << comp.scale;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::CameraComponent>()) {
			auto& comp = entity.GetComponent<flaw::CameraComponent>();
			out << YAML::Key << "CameraComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Perspective" << YAML::Value << comp.perspective;
			out << YAML::Key << "Fov" << YAML::Value << comp.fov;
			out << YAML::Key << "AspectRatio" << YAML::Value << comp.aspectRatio;
			out << YAML::Key << "NearClip" << YAML::Value << comp.nearClip;
			out << YAML::Key << "FarClip" << YAML::Value << comp.farClip;
			out << YAML::Key << "OrthoSize" << YAML::Value << comp.orthoSize;
			out << YAML::Key << "Depth" << YAML::Value << comp.depth;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::SpriteRendererComponent>()) {
			auto& comp = entity.GetComponent<flaw::SpriteRendererComponent>();
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Color" << YAML::Value << comp.color;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::Rigidbody2DComponent>()) {
			auto& comp = entity.GetComponent<flaw::Rigidbody2DComponent>();
			out << YAML::Key << "Rigidbody2DComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "BodyType" << YAML::Value << (int)comp.bodyType;
			out << YAML::Key << "FixedRotation" << YAML::Value << comp.fixedRotation;
			out << YAML::Key << "Density" << YAML::Value << comp.density;
			out << YAML::Key << "Friction" << YAML::Value << comp.friction;
			out << YAML::Key << "Restitution" << YAML::Value << comp.restitution;
			out << YAML::Key << "RestitutionThreshold" << YAML::Value << comp.restitutionThreshold;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::BoxCollider2DComponent>()) {
			auto& comp = entity.GetComponent<flaw::BoxCollider2DComponent>();
			out << YAML::Key << "BoxCollider2DComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Offset" << YAML::Value << comp.offset;
			out << YAML::Key << "Size" << YAML::Value << comp.size;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::MonoScriptComponent>()) {
			auto& comp = entity.GetComponent<flaw::MonoScriptComponent>();
			out << YAML::Key << "MonoScriptComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << comp.name;
			out << YAML::EndMap;
		}

		out << YAML::EndMap;
		out << YAML::EndMap;
	}

	void Serialize(YAML::Emitter& out, Scene& scene) {
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled";
		out << YAML::Key << "Entities";
		out << YAML::Value << YAML::BeginSeq;

		for (auto&& [entity] : scene.GetRegistry().view<entt::entity>().each()) {
			flaw::Entity e(entity, &scene);

			if (e) {
				Serialize(out, e);
			}
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;
	}

	void Deserialize(const YAML::Node& node, ProjectConfig& config) {
		auto root = node["ProjectConfig"];
		if (!root) {
			return;
		}

		config.name = root["Name"].as<std::string>();
		config.path = root["Path"].as<std::string>();
		config.startScene = root["StartScene"].as<std::string>();
	}

	void Deserialize(const YAML::Node& node, Entity& entity) {
		UUID id = node["Entity"].as<uint64_t>();

		auto components = node["Components"];

		if (components) {
			for (auto component : components) {
				std::string name = component.first.as<std::string>();

				if (name == "EntityComponent") {
					if (!entity.HasComponent<EntityComponent>()) {
						entity.AddComponent<EntityComponent>();
					}

					auto& comp = entity.GetComponent<EntityComponent>();
					comp.uuid = component.second["UUID"].as<uint64_t>();
					comp.name = component.second["Name"].as<std::string>();
				}
				else if (name == "TransformComponent") {
					if (!entity.HasComponent<TransformComponent>()) {
						entity.AddComponent<TransformComponent>();
					}

					auto& comp = entity.GetComponent<TransformComponent>();
					comp.position = component.second["Position"].as<vec3>();
					comp.rotation = component.second["Rotation"].as<vec3>();
					comp.scale = component.second["Scale"].as<vec3>();
				}
				else if (name == "CameraComponent") {
					if (!entity.HasComponent<CameraComponent>()) {
						entity.AddComponent<CameraComponent>();
					}

					auto& comp = entity.GetComponent<CameraComponent>();
					comp.perspective = component.second["Perspective"].as<bool>();
					comp.fov = component.second["Fov"].as<float>();
					comp.aspectRatio = component.second["AspectRatio"].as<float>();
					comp.nearClip = component.second["NearClip"].as<float>();
					comp.farClip = component.second["FarClip"].as<float>();
					comp.orthoSize = component.second["OrthoSize"].as<float>();
					comp.depth = component.second["Depth"].as<uint32_t>();
				}
				else if (name == "SpriteRendererComponent") {
					if (!entity.HasComponent<SpriteRendererComponent>()) {
						entity.AddComponent<SpriteRendererComponent>();
					}

					auto& comp = entity.GetComponent<SpriteRendererComponent>();
					comp.color = component.second["Color"].as<vec4>();
				}
				else if (name == "Rigidbody2DComponent") {
					if (!entity.HasComponent<Rigidbody2DComponent>()) {
						entity.AddComponent<Rigidbody2DComponent>();
					}
					auto& comp = entity.GetComponent<Rigidbody2DComponent>();
					comp.bodyType = (Rigidbody2DComponent::BodyType)component.second["BodyType"].as<int>();
					comp.fixedRotation = component.second["FixedRotation"].as<bool>();
					comp.density = component.second["Density"].as<float>();
					comp.friction = component.second["Friction"].as<float>();
					comp.restitution = component.second["Restitution"].as<float>();
					comp.restitutionThreshold = component.second["RestitutionThreshold"].as<float>();
				}
				else if (name == "BoxCollider2DComponent") {
					if (!entity.HasComponent<BoxCollider2DComponent>()) {
						entity.AddComponent<BoxCollider2DComponent>();
					}
					auto& comp = entity.GetComponent<BoxCollider2DComponent>();
					comp.offset = component.second["Offset"].as<vec2>();
					comp.size = component.second["Size"].as<vec2>();
				}
				else if (name == "MonoScriptComponent") {
					if (!entity.HasComponent<MonoScriptComponent>()) {
						entity.AddComponent<MonoScriptComponent>();
					}
					auto& comp = entity.GetComponent<MonoScriptComponent>();
					comp.name = component.second["Name"].as<std::string>();
				}
			}
		}
	}

	void Deserialize(const YAML::Node& node, Scene& scene) {
		std::string name = node["Scene"].as<std::string>();

		auto entities = node["Entities"];
		for (auto entity : entities) {
			Entity e = scene.CreateEntity();
			Deserialize(entity, e);
		}
	}
}

namespace YAML {
	Emitter& operator<<(Emitter& out, vec2 vec) {
		out << Flow;
		out << BeginSeq << vec.x << vec.y << EndSeq;
		return out;
	}

	Emitter& operator<<(Emitter& out, vec3& vec) {
		out << Flow;
		out << BeginSeq << vec.x << vec.y << vec.z << EndSeq;
		return out;
	}

	Emitter& operator<<(Emitter& out, vec4& vec) {
		out << Flow;
		out << BeginSeq << vec.x << vec.y << vec.z << vec.w << EndSeq;
		return out;
	}
}