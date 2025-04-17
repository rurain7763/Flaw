#include "pch.h"
#include "Serialization.h"
#include "Components.h"
#include "ECS/ECS.h"
#include "Project.h"
#include "Scene.h"
#include "Entity.h"
#include "AssetManager.h"
#include "ParticleSystem.h"

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
			out << YAML::Key << "Texture" << YAML::Value << comp.texture;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::Rigidbody2DComponent>()) {
			auto& comp = entity.GetComponent<flaw::Rigidbody2DComponent>();
			out << YAML::Key << "Rigidbody2DComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "BodyType" << YAML::Value << (int32_t)comp.bodyType;
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

		if (entity.HasComponent<flaw::CircleCollider2DComponent>()) {
			auto& comp = entity.GetComponent<flaw::CircleCollider2DComponent>();
			out << YAML::Key << "CircleCollider2DComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Offset" << YAML::Value << comp.offset;
			out << YAML::Key << "Radius" << YAML::Value << comp.radius;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::MonoScriptComponent>()) {
			auto& comp = entity.GetComponent<flaw::MonoScriptComponent>();
			out << YAML::Key << "MonoScriptComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << comp.name;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::TextComponent>()) {
			auto& comp = entity.GetComponent<flaw::TextComponent>();
			out << YAML::Key << "TextComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Text" << YAML::Value << Utf16ToUtf8(comp.text);
			out << YAML::Key << "Color" << YAML::Value << comp.color;
			out << YAML::Key << "Font" << YAML::Value << comp.font;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::SoundListenerComponent>()) {
			auto& comp = entity.GetComponent<flaw::SoundListenerComponent>();
			out << YAML::Key << "SoundListenerComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Velocity" << YAML::Value << comp.velocity;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::SoundSourceComponent>()) {
			auto& comp = entity.GetComponent<flaw::SoundSourceComponent>();
			out << YAML::Key << "SoundSourceComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Sound" << YAML::Value << comp.sound;
			out << YAML::Key << "Loop" << YAML::Value << comp.loop;
			out << YAML::Key << "AutoPlay" << YAML::Value << comp.autoPlay;
			out << YAML::Key << "Volume" << YAML::Value << comp.volume;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::MeshFilterComponent>()) {
			auto& comp = entity.GetComponent<flaw::MeshFilterComponent>();
			out << YAML::Key << "MeshFilterComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Mesh" << YAML::Value << comp.mesh;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::MeshRendererComponent>()) {
			auto& comp = entity.GetComponent<flaw::MeshRendererComponent>();
			out << YAML::Key << "MeshRendererComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Material" << YAML::Value << comp.material;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::ParticleComponent>()) {
			auto& comp = entity.GetComponent<flaw::ParticleComponent>();
			out << YAML::Key << "ParticleComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "MaxParticles" << YAML::Value << comp.maxParticles;
			out << YAML::Key << "SpaceType" << YAML::Value << (int32_t)comp.spaceType;
			out << YAML::Key << "StartSpeed" << YAML::Value << comp.startSpeed;
			out << YAML::Key << "StartLifeTime" << YAML::Value << comp.startLifeTime;
			out << YAML::Key << "StartColor" << YAML::Value << comp.startColor;
			out << YAML::Key << "StartSize" << YAML::Value << comp.startSize;
			out << YAML::Key << "Modules" << YAML::Value << comp.modules;

			auto& particleSys = entity.GetScene().GetParticleSystem();

			if (comp.modules & ParticleComponent::ModuleType::Emission) {
				auto module = particleSys.GetModule<EmissionModule>(entity);

				out << YAML::Key << "EmissionModule";
				out << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "SpawnOverTime" << YAML::Value << module->spawnOverTime;
				out << YAML::Key << "Burst" << YAML::Value << module->burst;
				out << YAML::Key << "BurstStartTime" << YAML::Value << module->burstStartTime;
				out << YAML::Key << "BurstParticleCount" << YAML::Value << module->burstParticleCount;
				out << YAML::Key << "BurstCycleCount" << YAML::Value << module->burstCycleCount;
				out << YAML::Key << "BurstCycleInterval" << YAML::Value << module->burstCycleInterval;
				out << YAML::EndMap;
			}

			if (comp.modules & ParticleComponent::ModuleType::Shape) {
				auto module = particleSys.GetModule<ShapeModule>(entity);

				out << YAML::Key << "ShapeModule";
				out << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "ShapeType" << YAML::Value << (int32_t)module->shapeType;
				if (module->shapeType == ShapeModule::ShapeType::Sphere) {
					out << YAML::Key << "Radius" << YAML::Value << module->sphere.radius;
					out << YAML::Key << "Thickness" << YAML::Value << module->sphere.thickness;
				}
				else if (module->shapeType == ShapeModule::ShapeType::Box) {
					out << YAML::Key << "Size" << YAML::Value << module->box.size;
					out << YAML::Key << "Thickness" << YAML::Value << module->box.thickness;
				}
				out << YAML::EndMap;
			}

			if (comp.modules & ParticleComponent::ModuleType::RandomSpeed) {
				auto module = particleSys.GetModule<RandomSpeedModule>(entity);

				out << YAML::Key << "RandomSpeedModule";
				out << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "MinSpeed" << YAML::Value << module->minSpeed;
				out << YAML::Key << "MaxSpeed" << YAML::Value << module->maxSpeed;
				out << YAML::EndMap;
			}

			if (comp.modules & ParticleComponent::ModuleType::RandomColor) {
				auto module = particleSys.GetModule<RandomColorModule>(entity);

				out << YAML::Key << "RandomColorModule";
				out << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "MinColor" << YAML::Value << module->minColor;
				out << YAML::Key << "MaxColor" << YAML::Value << module->maxColor;
				out << YAML::EndMap;
			}

			if (comp.modules & ParticleComponent::ModuleType::RandomSize) {
				auto module = particleSys.GetModule<RandomSizeModule>(entity);

				out << YAML::Key << "RandomSizeModule";
				out << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "MinSize" << YAML::Value << module->minSize;
				out << YAML::Key << "MaxSize" << YAML::Value << module->maxSize;
				out << YAML::EndMap;
			}

			if (comp.modules & ParticleComponent::ModuleType::ColorOverLifetime) {
				auto module = particleSys.GetModule<ColorOverLifetimeModule>(entity);
				out << YAML::Key << "ColorOverLifetimeModule";
				out << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "Easing" << YAML::Value << (int32_t)module->easing;
				out << YAML::Key << "EasingStartRatio" << YAML::Value << module->easingStartRatio;
				out << YAML::Key << "RedFactorRange" << YAML::Value << module->redFactorRange;
				out << YAML::Key << "GreenFactorRange" << YAML::Value << module->greenFactorRange;
				out << YAML::Key << "BlueFactorRange" << YAML::Value << module->blueFactorRange;
				out << YAML::Key << "AlphaFactorRange" << YAML::Value << module->alphaFactorRange;
				out << YAML::EndMap;
			}

			if (comp.modules & ParticleComponent::ModuleType::SizeOverLifetime) {
				auto module = particleSys.GetModule<SizeOverLifetimeModule>(entity);
				out << YAML::Key << "SizeOverLifetimeModule";
				out << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "Easing" << YAML::Value << (int32_t)module->easing;
				out << YAML::Key << "EasingStartRatio" << YAML::Value << module->easingStartRatio;
				out << YAML::Key << "SizeFactorRange" << YAML::Value << module->sizeFactorRange;
				out << YAML::EndMap;
			}

			if (comp.modules & ParticleComponent::ModuleType::Noise) {
				auto module = particleSys.GetModule<NoiseModule>(entity);
				out << YAML::Key << "NoiseModule";
				out << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "Strength" << YAML::Value << module->strength;
				out << YAML::Key << "Frequency" << YAML::Value << module->frequency;
				out << YAML::EndMap;
			}

			if (comp.modules & ParticleComponent::ModuleType::Renderer) {
				auto module = particleSys.GetModule<RendererModule>(entity);
				out << YAML::Key << "RendererModule";
				out << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "Alignment" << YAML::Value << (int32_t)module->alignment;
				out << YAML::EndMap;
			}

			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::SkyLightComponent>()) {
			auto& comp = entity.GetComponent<flaw::SkyLightComponent>();
			out << YAML::Key << "SkyLightComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Color" << YAML::Value << comp.color;
			out << YAML::Key << "Intensity" << YAML::Value << comp.intensity;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::DirectionalLightComponent>()) {
			auto& comp = entity.GetComponent<flaw::DirectionalLightComponent>();
			out << YAML::Key << "DirectionalLightComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Color" << YAML::Value << comp.color;
			out << YAML::Key << "Intensity" << YAML::Value << comp.intensity;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::PointLightComponent>()) {
			auto& comp = entity.GetComponent<flaw::PointLightComponent>();
			out << YAML::Key << "PointLightComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Color" << YAML::Value << comp.color;
			out << YAML::Key << "Intensity" << YAML::Value << comp.intensity;
			out << YAML::Key << "Range" << YAML::Value << comp.range;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::SpotLightComponent>()) {
			auto& comp = entity.GetComponent<flaw::SpotLightComponent>();
			out << YAML::Key << "SpotLightComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Color" << YAML::Value << comp.color;
			out << YAML::Key << "Intensity" << YAML::Value << comp.intensity;
			out << YAML::Key << "Range" << YAML::Value << comp.range;
			out << YAML::Key << "Inner" << YAML::Value << comp.inner;
			out << YAML::Key << "Outer" << YAML::Value << comp.outer;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::SkyBoxComponent>()) {
			auto& comp = entity.GetComponent<flaw::SkyBoxComponent>();
			out << YAML::Key << "SkyBoxComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Texture" << YAML::Value << comp.texture;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::DecalComponent>()) {
			auto& comp = entity.GetComponent<flaw::DecalComponent>();
			out << YAML::Key << "DecalComponent";
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Texture" << YAML::Value << comp.texture;
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

		std::unordered_map<UUID, UUID> parentMap;
		for (auto&& [entity] : scene.GetRegistry().view<entt::entity>().each()) {
			flaw::Entity e(entity, &scene);

			if (e) {
				Serialize(out, e);
			}

			if (e.HasParent()) {
				parentMap[e.GetUUID()] = e.GetParent().GetUUID();
			}
		}

		out << YAML::EndSeq;

		out << YAML::Key << "ParentMap";
		out << YAML::Value << YAML::BeginMap;
		{
			for (auto&& [child, parent] : parentMap) {
				out << YAML::Key << child << YAML::Value << parent;
			}
		}
		out << YAML::EndMap;

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
		auto components = node["Components"];

		if (components) {
			for (auto component : components) {
				std::string name = component.first.as<std::string>();

				if (name == "EntityComponent") {
					if (!entity.HasComponent<EntityComponent>()) {
						entity.AddComponent<EntityComponent>();
					}

					auto& comp = entity.GetComponent<EntityComponent>();
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
					comp.texture = component.second["Texture"].as<uint64_t>();
				}
				else if (name == "Rigidbody2DComponent") {
					if (!entity.HasComponent<Rigidbody2DComponent>()) {
						entity.AddComponent<Rigidbody2DComponent>();
					}
					auto& comp = entity.GetComponent<Rigidbody2DComponent>();
					comp.bodyType = (Rigidbody2DComponent::BodyType)component.second["BodyType"].as<int32_t>();
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
				else if (name == "CircleCollider2DComponent") {
					if (!entity.HasComponent<CircleCollider2DComponent>()) {
						entity.AddComponent<CircleCollider2DComponent>();
					}
					auto& comp = entity.GetComponent<CircleCollider2DComponent>();
					comp.offset = component.second["Offset"].as<vec2>();
					comp.radius = component.second["Radius"].as<float>();
				}
				else if (name == "MonoScriptComponent") {
					if (!entity.HasComponent<MonoScriptComponent>()) {
						entity.AddComponent<MonoScriptComponent>();
					}
					auto& comp = entity.GetComponent<MonoScriptComponent>();
					comp.name = component.second["Name"].as<std::string>();
				}
				else if (name == "TextComponent") {
					if (!entity.HasComponent<TextComponent>()) {
						entity.AddComponent<TextComponent>();
					}
					auto& comp = entity.GetComponent<TextComponent>();
					comp.text = Utf8ToUtf16(component.second["Text"].as<std::string>());
					comp.color = component.second["Color"].as<vec4>();
					comp.font = component.second["Font"].as<uint64_t>();
				}
				else if (name == "SoundListenerComponent") {
					if (!entity.HasComponent<SoundListenerComponent>()) {
						entity.AddComponent<SoundListenerComponent>();
					}
					auto& comp = entity.GetComponent<SoundListenerComponent>();
					comp.velocity = component.second["Velocity"].as<vec3>();
				}
				else if (name == "SoundSourceComponent") {
					if (!entity.HasComponent<SoundSourceComponent>()) {
						entity.AddComponent<SoundSourceComponent>();
					}
					auto& comp = entity.GetComponent<SoundSourceComponent>();
					comp.sound = component.second["Sound"].as<uint64_t>();
					comp.loop = component.second["Loop"].as<bool>();
					comp.autoPlay = component.second["AutoPlay"].as<bool>();
					comp.volume = component.second["Volume"].as<float>();
				}
				else if (name == "MeshFilterComponent") {
					if (!entity.HasComponent<MeshFilterComponent>()) {
						entity.AddComponent<MeshFilterComponent>();
					}

					auto& comp = entity.GetComponent<MeshFilterComponent>();
					comp.mesh = component.second["Mesh"].as<uint64_t>();
				}
				else if (name == "MeshRendererComponent") {
					if (!entity.HasComponent<MeshRendererComponent>()) {
						entity.AddComponent<MeshRendererComponent>();
					}

					auto& comp = entity.GetComponent<MeshRendererComponent>();
					comp.material = component.second["Material"].as<uint64_t>();
				}
				else if (name == "ParticleComponent") {
					if (!entity.HasComponent<ParticleComponent>()) {
						entity.AddComponent<ParticleComponent>();
					}

					auto& comp = entity.GetComponent<ParticleComponent>();

					comp.maxParticles = component.second["MaxParticles"].as<uint32_t>();
					comp.spaceType = (ParticleComponent::SpaceType)component.second["SpaceType"].as<int32_t>();
					comp.startSpeed = component.second["StartSpeed"].as<float>();
					comp.startLifeTime = component.second["StartLifeTime"].as<float>();
					comp.startColor = component.second["StartColor"].as<vec4>();
					comp.startSize = component.second["StartSize"].as<vec3>();
					comp.modules = component.second["Modules"].as<uint32_t>();

					auto& particleSys = entity.GetScene().GetParticleSystem();

					if (comp.modules & ParticleComponent::ModuleType::Emission) {
						auto module = particleSys.AddModule<EmissionModule>(entity);
						auto moduleNode = component.second["EmissionModule"];

						module->spawnOverTime = moduleNode["SpawnOverTime"].as<int32_t>();
						module->burst = moduleNode["Burst"].as<bool>();
						module->burstStartTime = moduleNode["BurstStartTime"].as<float>();
						module->burstParticleCount = moduleNode["BurstParticleCount"].as<uint32_t>();
						module->burstCycleCount = moduleNode["BurstCycleCount"].as<uint32_t>();
						module->burstCycleInterval = moduleNode["BurstCycleInterval"].as<float>();
					}

					if (comp.modules & ParticleComponent::ModuleType::Shape) {
						auto module = particleSys.AddModule<ShapeModule>(entity);
						auto moduleNode = component.second["ShapeModule"];

						module->shapeType = (ShapeModule::ShapeType)moduleNode["ShapeType"].as<int32_t>();
						if (module->shapeType == ShapeModule::ShapeType::Sphere) {
							module->sphere.radius = moduleNode["Radius"].as<float>();
							module->sphere.thickness = moduleNode["Thickness"].as<float>();
						}
						else if (module->shapeType == ShapeModule::ShapeType::Box) {
							module->box.size = moduleNode["Size"].as<vec3>();
							module->box.thickness = moduleNode["Thickness"].as<vec3>();
						}
					}

					if (comp.modules & ParticleComponent::ModuleType::RandomSpeed) {
						auto module = particleSys.AddModule<RandomSpeedModule>(entity);
						auto moduleNode = component.second["RandomSpeedModule"];

						module->minSpeed = moduleNode["MinSpeed"].as<float>();
						module->maxSpeed = moduleNode["MaxSpeed"].as<float>();
					}

					if (comp.modules & ParticleComponent::ModuleType::RandomColor) {
						auto module = particleSys.AddModule<RandomColorModule>(entity);
						auto moduleNode = component.second["RandomColorModule"];

						module->minColor = moduleNode["MinColor"].as<vec4>();
						module->maxColor = moduleNode["MaxColor"].as<vec4>();
					}

					if (comp.modules & ParticleComponent::ModuleType::RandomSize) {
						auto module = particleSys.AddModule<RandomSizeModule>(entity);
						auto moduleNode = component.second["RandomSizeModule"];

						module->minSize = moduleNode["MinSize"].as<vec3>();
						module->maxSize = moduleNode["MaxSize"].as<vec3>();
					}

					if (comp.modules & ParticleComponent::ModuleType::ColorOverLifetime) {
						auto module = particleSys.AddModule<ColorOverLifetimeModule>(entity);
						auto moduleNode = component.second["ColorOverLifetimeModule"];

						module->easing = (Easing)moduleNode["Easing"].as<int32_t>();
						module->easingStartRatio = moduleNode["EasingStartRatio"].as<float>();
						module->redFactorRange = moduleNode["RedFactorRange"].as<vec2>();
						module->greenFactorRange = moduleNode["GreenFactorRange"].as<vec2>();
						module->blueFactorRange = moduleNode["BlueFactorRange"].as<vec2>();
						module->alphaFactorRange = moduleNode["AlphaFactorRange"].as<vec2>();
					}

					if (comp.modules & ParticleComponent::ModuleType::SizeOverLifetime) {
						auto module = particleSys.AddModule<SizeOverLifetimeModule>(entity);
						auto moduleNode = component.second["SizeOverLifetimeModule"];

						module->easing = (Easing)moduleNode["Easing"].as<int32_t>();
						module->easingStartRatio = moduleNode["EasingStartRatio"].as<float>();
						module->sizeFactorRange = moduleNode["SizeFactorRange"].as<vec2>();
					}

					if (comp.modules & ParticleComponent::ModuleType::Noise) {
						auto module = particleSys.AddModule<NoiseModule>(entity);
						auto moduleNode = component.second["NoiseModule"];

						module->strength = moduleNode["Strength"].as<float>();
						module->frequency = moduleNode["Frequency"].as<float>();
					}

					if (comp.modules & ParticleComponent::ModuleType::Renderer) {
						auto module = particleSys.AddModule<RendererModule>(entity);
						auto moduleNode = component.second["RendererModule"];

						module->alignment = (RendererModule::Alignment)moduleNode["Alignment"].as<int32_t>();
					}
				}
				else if (name == "SkyLightComponent") {
					if (!entity.HasComponent<SkyLightComponent>()) {
						entity.AddComponent<SkyLightComponent>();
					}

					auto& comp = entity.GetComponent<SkyLightComponent>();
					comp.color = component.second["Color"].as<vec3>();
					comp.intensity = component.second["Intensity"].as<float>();
				}
				else if (name == "DirectionalLightComponent") {
					if (!entity.HasComponent<DirectionalLightComponent>()) {
						entity.AddComponent<DirectionalLightComponent>();
					}

					auto& comp = entity.GetComponent<DirectionalLightComponent>();
					comp.color = component.second["Color"].as<vec3>();
					comp.intensity = component.second["Intensity"].as<float>();
				}
				else if (name == "PointLightComponent") {
					if (!entity.HasComponent<PointLightComponent>()) {
						entity.AddComponent<PointLightComponent>();
					}

					auto& comp = entity.GetComponent<PointLightComponent>();
					comp.color = component.second["Color"].as<vec3>();
					comp.intensity = component.second["Intensity"].as<float>();
					comp.range = component.second["Range"].as<float>();
				}
				else if (name == "SpotLightComponent") {
					if (!entity.HasComponent<SpotLightComponent>()) {
						entity.AddComponent<SpotLightComponent>();
					}
					auto& comp = entity.GetComponent<SpotLightComponent>();
					comp.color = component.second["Color"].as<vec3>();
					comp.intensity = component.second["Intensity"].as<float>();
					comp.range = component.second["Range"].as<float>();
					comp.inner = component.second["Inner"].as<float>();
					comp.outer = component.second["Outer"].as<float>();
				}
				else if (name == "SkyBoxComponent") {
					if (!entity.HasComponent<SkyBoxComponent>()) {
						entity.AddComponent<SkyBoxComponent>();
					}
					auto& comp = entity.GetComponent<SkyBoxComponent>();
					comp.texture = component.second["Texture"].as<uint64_t>();
				}
				else if (name == "DecalComponent") {
					if (!entity.HasComponent<DecalComponent>()) {
						entity.AddComponent<DecalComponent>();
					}
					auto& comp = entity.GetComponent<DecalComponent>();
					comp.texture = component.second["Texture"].as<uint64_t>();
				}
			}
		}
	}

	void Deserialize(const YAML::Node& node, Scene& scene) {
		std::string name = node["Scene"].as<std::string>();

		auto entities = node["Entities"];
		for (auto node : entities) {
			UUID id = node["Entity"].as<uint64_t>();
			Entity e = scene.CreateEntityByUUID(id);

			Deserialize(node, e);
		}

		auto parentMap = node["ParentMap"];
		for (auto parent : parentMap) {
			auto child = parent.first.as<uint64_t>();
			auto parentUUID = parent.second.as<uint64_t>();

			Entity childEntity = scene.FindEntityByUUID(child);
			Entity parentEntity = scene.FindEntityByUUID(parentUUID);

			if (childEntity && parentEntity) {
				childEntity.SetParent(parentEntity);
			}
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