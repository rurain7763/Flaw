#include "pch.h"
#include "Serialization.h"
#include "Components.h"
#include "ECS/ECS.h"
#include "Project.h"
#include "Scene.h"
#include "Entity.h"
#include "AssetManager.h"
#include "ParticleSystem.h"
#include "Scripting.h"
#include "MonoScriptSystem.h"

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
			out << YAML::Key << TypeName<flaw::EntityComponent>().data();
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << comp.name;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::TransformComponent>()) {
			auto& comp = entity.GetComponent<flaw::TransformComponent>();
			out << YAML::Key << TypeName<flaw::TransformComponent>().data();
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

		if (entity.HasComponent<flaw::RigidbodyComponent>()) {
			auto& comp = entity.GetComponent<flaw::RigidbodyComponent>();
			out << YAML::Key << TypeName<flaw::RigidbodyComponent>().data();
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "BodyType" << YAML::Value << (int32_t)comp.bodyType;
			out << YAML::Key << "IsKinematic" << YAML::Value << comp.isKinematic;
			out << YAML::Key << "Mass" << YAML::Value << comp.mass;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::BoxColliderComponent>()) {
			auto& comp = entity.GetComponent<flaw::BoxColliderComponent>();
			out << YAML::Key << TypeName<flaw::BoxColliderComponent>().data();
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "IsTrigger" << YAML::Value << comp.isTrigger;
			out << YAML::Key << "StaticFriction" << YAML::Value << comp.staticFriction;
			out << YAML::Key << "DynamicFriction" << YAML::Value << comp.dynamicFriction;
			out << YAML::Key << "Restitution" << YAML::Value << comp.restitution;
			out << YAML::Key << "Offset" << YAML::Value << comp.offset;
			out << YAML::Key << "Size" << YAML::Value << comp.size;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::SphereColliderComponent>()) {
			auto& comp = entity.GetComponent<flaw::SphereColliderComponent>();
			out << YAML::Key << TypeName<flaw::SphereColliderComponent>().data();
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "IsTrigger" << YAML::Value << comp.isTrigger;
			out << YAML::Key << "StaticFriction" << YAML::Value << comp.staticFriction;
			out << YAML::Key << "DynamicFriction" << YAML::Value << comp.dynamicFriction;
			out << YAML::Key << "Restitution" << YAML::Value << comp.restitution;
			out << YAML::Key << "Offset" << YAML::Value << comp.offset;
			out << YAML::Key << "Radius" << YAML::Value << comp.radius;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::MeshColliderComponent>()) {
			auto& comp = entity.GetComponent<flaw::MeshColliderComponent>();
			out << YAML::Key << TypeName<flaw::MeshColliderComponent>().data();
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "IsTrigger" << YAML::Value << comp.isTrigger;
			out << YAML::Key << "StaticFriction" << YAML::Value << comp.staticFriction;
			out << YAML::Key << "DynamicFriction" << YAML::Value << comp.dynamicFriction;
			out << YAML::Key << "Restitution" << YAML::Value << comp.restitution;
			out << YAML::Key << "Mesh" << YAML::Value << comp.mesh;
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

		if (entity.HasComponent<flaw::StaticMeshComponent>()) {
			auto& comp = entity.GetComponent<flaw::StaticMeshComponent>();
			out << YAML::Key << TypeName<flaw::StaticMeshComponent>().data();
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Mesh" << YAML::Value << comp.mesh;
			out << YAML::Key << "Materials" << YAML::Value << YAML::BeginSeq;
			for (const auto& mat : comp.materials) {
				out << mat;
			}
			out << YAML::EndSeq;
			out << YAML::Key << "CastShadow" << YAML::Value << comp.castShadow;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::SkeletalMeshComponent>()) {
			auto& comp = entity.GetComponent<flaw::SkeletalMeshComponent>();
			out << YAML::Key << TypeName<flaw::SkeletalMeshComponent>().data();
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Mesh" << YAML::Value << comp.mesh;
			out << YAML::Key << "Materials" << YAML::Value << YAML::BeginSeq;
			for (const auto& mat : comp.materials) {
				out << mat;
			}
			out << YAML::EndSeq;
			out << YAML::Key << "Skeleton" << YAML::Value << comp.skeleton;
			out << YAML::Key << "CastShadow" << YAML::Value << comp.castShadow;
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

		if (entity.HasComponent<flaw::LandscapeComponent>()) {
			auto& comp = entity.GetComponent<flaw::LandscapeComponent>();
			out << YAML::Key << TypeName<flaw::LandscapeComponent>().data();
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "TilingX" << YAML::Value << comp.tilingX;
			out << YAML::Key << "TilingY" << YAML::Value << comp.tilingY;
			out << YAML::Key << "HeightMap" << YAML::Value << comp.heightMap;
			out << YAML::Key << "LODLevelMax" << YAML::Value << comp.lodLevelMax;
			out << YAML::Key << "LODDistanceRange" << YAML::Value << comp.lodDistanceRange;
			out << YAML::Key << "AlbedoTexture2DArray" << YAML::Value << comp.albedoTexture2DArray;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::AnimatorComponent>()) {
			auto& comp = entity.GetComponent<flaw::AnimatorComponent>();
			out << YAML::Key << TypeName<flaw::AnimatorComponent>().data();
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "SkeletonAsset" << YAML::Value << comp.skeletonAsset;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::CanvasComponent>()) {
			auto& comp = entity.GetComponent<flaw::CanvasComponent>();
			out << YAML::Key << TypeName<flaw::CanvasComponent>().data();
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "RenderMode" << YAML::Value << (int32_t)comp.renderMode;
			out << YAML::Key << "RenderCamera" << YAML::Value << comp.renderCamera;
			out << YAML::Key << "PlaneDistance" << YAML::Value << comp.planeDistance;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::CanvasScalerComponent>()) {
			auto& comp = entity.GetComponent<flaw::CanvasScalerComponent>();
			out << YAML::Key << TypeName<flaw::CanvasScalerComponent>().data();
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "ScaleMode" << YAML::Value << (int32_t)comp.scaleMode;
			out << YAML::Key << "ScaleFactor" << YAML::Value << comp.scaleFactor;
			out << YAML::Key << "ReferenceResolution" << YAML::Value << comp.referenceResolution;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::RectLayoutComponent>()) {
			auto& comp = entity.GetComponent<flaw::RectLayoutComponent>();
			out << YAML::Key << TypeName<flaw::RectLayoutComponent>().data();
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "AnchorMin" << YAML::Value << comp.anchorMin;
			out << YAML::Key << "AnchorMax" << YAML::Value << comp.anchorMax;
			out << YAML::Key << "Pivot" << YAML::Value << comp.pivot;
			out << YAML::Key << "SizeDelta" << YAML::Value << comp.sizeDelta;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::ImageComponent>()) {
			auto& comp = entity.GetComponent<flaw::ImageComponent>();
			out << YAML::Key << TypeName<flaw::ImageComponent>().data();
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Texture" << YAML::Value << comp.texture;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<flaw::MonoScriptComponent>()) {
			auto& comp = entity.GetComponent<flaw::MonoScriptComponent>();
			out << YAML::Key << TypeName<flaw::MonoScriptComponent>().data();
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << comp.name;

			out << YAML::Key << "Fields" << YAML::Value << YAML::BeginMap;

			for (const auto& field : comp.fields) {
				out << YAML::Key << field.fieldName << YAML::BeginSeq;
				out << YAML::Value << field.fieldType << YAML::Value << field.fieldValue;
				out << YAML::EndSeq;
			}

			out << YAML::EndMap;

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

	void DeserializeEntityComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.GetComponent<EntityComponent>();
		comp.name = component.second["Name"].as<std::string>();
	}

	void DeserializeTransformComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.GetComponent<TransformComponent>();
		comp.position = component.second["Position"].as<vec3>();
		comp.rotation = component.second["Rotation"].as<vec3>();
		comp.scale = component.second["Scale"].as<vec3>();
	}

	void DeserializeCameraComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<CameraComponent>();
		comp.perspective = component.second["Perspective"].as<bool>();
		comp.fov = component.second["Fov"].as<float>();
		comp.aspectRatio = component.second["AspectRatio"].as<float>();
		comp.nearClip = component.second["NearClip"].as<float>();
		comp.farClip = component.second["FarClip"].as<float>();
		comp.orthoSize = component.second["OrthoSize"].as<float>();
		comp.depth = component.second["Depth"].as<uint32_t>();
	}

	void DeserializeSpriteRendererComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<SpriteRendererComponent>();
		comp.color = component.second["Color"].as<vec4>();
		comp.texture = component.second["Texture"].as<uint64_t>();
	}

	void DeserializeRigidbody2DComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<Rigidbody2DComponent>();
		comp.bodyType = (Rigidbody2DComponent::BodyType)component.second["BodyType"].as<int32_t>();
		comp.fixedRotation = component.second["FixedRotation"].as<bool>();
		comp.density = component.second["Density"].as<float>();
		comp.friction = component.second["Friction"].as<float>();
		comp.restitution = component.second["Restitution"].as<float>();
		comp.restitutionThreshold = component.second["RestitutionThreshold"].as<float>();
	}

	void DeserializeBoxCollider2DComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<BoxCollider2DComponent>();
		comp.offset = component.second["Offset"].as<vec2>();
		comp.size = component.second["Size"].as<vec2>();
	}

	void DeserializeCircleCollider2DComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<CircleCollider2DComponent>();
		comp.offset = component.second["Offset"].as<vec2>();
		comp.radius = component.second["Radius"].as<float>();
	}

	void DeserializeRigidbodyComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<RigidbodyComponent>();
		comp.bodyType = (PhysicsBodyType)component.second["BodyType"].as<int32_t>();
		comp.isKinematic = component.second["IsKinematic"].as<bool>();
		comp.mass = component.second["Mass"].as<float>();
	}

	void DeserializeBoxColliderComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<BoxColliderComponent>();
		comp.isTrigger = component.second["IsTrigger"].as<bool>();
		comp.staticFriction = component.second["StaticFriction"].as<float>();
		comp.dynamicFriction = component.second["DynamicFriction"].as<float>();
		comp.restitution = component.second["Restitution"].as<float>();
		comp.offset = component.second["Offset"].as<vec3>();
		comp.size = component.second["Size"].as<vec3>();
	}

	void DeserializeSphereColliderComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<SphereColliderComponent>();
		comp.isTrigger = component.second["IsTrigger"].as<bool>();
		comp.staticFriction = component.second["StaticFriction"].as<float>();
		comp.dynamicFriction = component.second["DynamicFriction"].as<float>();
		comp.restitution = component.second["Restitution"].as<float>();
		comp.offset = component.second["Offset"].as<vec3>();
		comp.radius = component.second["Radius"].as<float>();
	}

	void DeserializeMeshColliderComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<MeshColliderComponent>();
		comp.isTrigger = component.second["IsTrigger"].as<bool>();
		comp.staticFriction = component.second["StaticFriction"].as<float>();
		comp.dynamicFriction = component.second["DynamicFriction"].as<float>();
		comp.restitution = component.second["Restitution"].as<float>();
		comp.mesh = component.second["Mesh"].as<uint64_t>();
	}

	void DeserializeTextComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<TextComponent>();
		comp.text = Utf8ToUtf16(component.second["Text"].as<std::string>());
		comp.color = component.second["Color"].as<vec4>();
		comp.font = component.second["Font"].as<uint64_t>();
	}

	void DeserializeSoundListenerComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<SoundListenerComponent>();
		comp.velocity = component.second["Velocity"].as<vec3>();
	}

	void DeserializeSoundSourceComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<SoundSourceComponent>();
		comp.sound = component.second["Sound"].as<uint64_t>();
		comp.loop = component.second["Loop"].as<bool>();
		comp.autoPlay = component.second["AutoPlay"].as<bool>();
		comp.volume = component.second["Volume"].as<float>();
	}

	void DeserializeStaticMeshComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<StaticMeshComponent>();
		comp.mesh = component.second["Mesh"].as<uint64_t>();
		comp.materials.clear();
		for (const auto& mat : component.second["Materials"]) {
			comp.materials.push_back(mat.as<uint64_t>());
		}
		comp.castShadow = component.second["CastShadow"].as<bool>();
	}

	void DeserializeSkeletalMeshComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<SkeletalMeshComponent>();
		comp.mesh = component.second["Mesh"].as<uint64_t>();
		comp.materials.clear();
		for (const auto& mat : component.second["Materials"]) {
			comp.materials.push_back(mat.as<uint64_t>());
		}
		comp.skeleton = component.second["Skeleton"].as<uint64_t>();
		comp.castShadow = component.second["CastShadow"].as<bool>();
	}

	void DeserializeParticleComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<ParticleComponent>();

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

	void DeserializeSkyLightComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<SkyLightComponent>();
		comp.color = component.second["Color"].as<vec3>();
		comp.intensity = component.second["Intensity"].as<float>();
	}

	void DeserializeDirectionalLightComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<DirectionalLightComponent>();
		comp.color = component.second["Color"].as<vec3>();
		comp.intensity = component.second["Intensity"].as<float>();
	}

	void DeserializePointLightComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<PointLightComponent>();
		comp.color = component.second["Color"].as<vec3>();
		comp.intensity = component.second["Intensity"].as<float>();
		comp.range = component.second["Range"].as<float>();
	}

	void DeserializeSpotLightComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<SpotLightComponent>();
		comp.color = component.second["Color"].as<vec3>();
		comp.intensity = component.second["Intensity"].as<float>();
		comp.range = component.second["Range"].as<float>();
		comp.inner = component.second["Inner"].as<float>();
		comp.outer = component.second["Outer"].as<float>();
	}

	void DeserializeSkyBoxComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<SkyBoxComponent>();
		comp.texture = component.second["Texture"].as<uint64_t>();
	}

	void DeserializeDecalComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<DecalComponent>();
		comp.texture = component.second["Texture"].as<uint64_t>();
	}

	void DeserializeLandscapeComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<LandscapeComponent>();
		comp.tilingX = component.second["TilingX"].as<float>();
		comp.tilingY = component.second["TilingY"].as<float>();
		comp.heightMap = component.second["HeightMap"].as<uint64_t>();
		comp.lodLevelMax = component.second["LODLevelMax"].as<uint32_t>();
		comp.lodDistanceRange = component.second["LODDistanceRange"].as<vec2>();
		comp.albedoTexture2DArray = component.second["AlbedoTexture2DArray"].as<uint64_t>();
	}

	void DeserializeAnimatorComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<AnimatorComponent>();
		comp.skeletonAsset = component.second["SkeletonAsset"].as<uint64_t>();
	}

	void DeserializeMonoScriptComponent(const YAML::iterator::value_type& component, Entity& entity) {
		std::string scriptName = component.second["Name"].as<std::string>();
		std::vector<MonoScriptComponent::FieldInfo> fields;

		auto fieldsNode = component.second["Fields"];
		for (const auto& field : fieldsNode) {
			MonoScriptComponent::FieldInfo fieldInfo;
			fieldInfo.fieldName = field.first.as<std::string>();
			fieldInfo.fieldType = field.second[0].as<std::string>();
			fieldInfo.fieldValue = field.second[1].as<std::string>();
			fields.emplace_back(std::move(fieldInfo));
		}

		entity.AddComponent<MonoScriptComponent>(scriptName.data(), fields);
	}

	void DeserializeCanvasComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<CanvasComponent>();
		comp.renderMode = (CanvasComponent::RenderMode)component.second["RenderMode"].as<int32_t>();
		comp.renderCamera = component.second["RenderCamera"].as<uint64_t>();
		comp.planeDistance = component.second["PlaneDistance"].as<float>();
	}

	void DeserializeCanvasScalerComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<CanvasScalerComponent>();
		comp.scaleMode = (CanvasScalerComponent::ScaleMode)component.second["ScaleMode"].as<int32_t>();
		comp.scaleFactor = component.second["ScaleFactor"].as<float>();
		comp.referenceResolution = component.second["ReferenceResolution"].as<vec2>();
	}

	void DeserializeRectLayoutComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<RectLayoutComponent>();
		comp.anchorMin = component.second["AnchorMin"].as<vec2>();
		comp.anchorMax = component.second["AnchorMax"].as<vec2>();
		comp.pivot = component.second["Pivot"].as<vec2>();
		comp.sizeDelta = component.second["SizeDelta"].as<vec2>();
	}

	void DeserializeImageComponent(const YAML::iterator::value_type& component, Entity& entity) {
		auto& comp = entity.AddComponent<ImageComponent>();
		comp.texture = component.second["Texture"].as<uint64_t>();
	}

	void Deserialize(const YAML::Node& node, Entity& entity, const std::unordered_map<std::string, std::function<void(const YAML::iterator::value_type&, Entity&)>>& userComponentDeserializer) {
		auto components = node["Components"];

		if (components) {
			std::string entityName;
			for (auto component : components) {
				std::string name = component.first.as<std::string>();

				auto it = userComponentDeserializer.find(name);
				if (it != userComponentDeserializer.end()) {
					it->second(component, entity);
					continue;
				}

				if (name == TypeName<EntityComponent>()) {
					DeserializeEntityComponent(component, entity);
				}
				else if (name == TypeName<TransformComponent>()) {
					DeserializeTransformComponent(component, entity);
				}
				else if (name == TypeName<CameraComponent>()) {
					DeserializeCameraComponent(component, entity);
				}
				else if (name == TypeName<SpriteRendererComponent>()) {
					DeserializeSpriteRendererComponent(component, entity);
				}
				else if (name == TypeName<Rigidbody2DComponent>()) {
					DeserializeRigidbody2DComponent(component, entity);
				}
				else if (name == TypeName<BoxCollider2DComponent>()) {
					DeserializeBoxCollider2DComponent(component, entity);
				}
				else if (name == TypeName<CircleCollider2DComponent>()) {
					DeserializeCircleCollider2DComponent(component, entity);
				}
				else if (name == TypeName<RigidbodyComponent>()) {
					DeserializeRigidbodyComponent(component, entity);
				}
				else if (name == TypeName<BoxColliderComponent>()) {
					DeserializeBoxColliderComponent(component, entity);
				}
				else if (name == TypeName<SphereColliderComponent>()) {
					DeserializeSphereColliderComponent(component, entity);
				}
				else if (name == TypeName<MeshColliderComponent>()) {
					DeserializeMeshColliderComponent(component, entity);
				}
				else if (name == TypeName<TextComponent>()) {
					DeserializeTextComponent(component, entity);
				}
				else if (name == TypeName<SoundListenerComponent>()) {
					DeserializeSoundListenerComponent(component, entity);
				}
				else if (name == TypeName<SoundSourceComponent>()) {
					DeserializeSoundSourceComponent(component, entity);
				}
				else if (name == TypeName<StaticMeshComponent>()) {
					DeserializeStaticMeshComponent(component, entity);
				}
				else if (name == TypeName<SkeletalMeshComponent>()) {
					DeserializeSkeletalMeshComponent(component, entity);
				}
				else if (name == TypeName<ParticleComponent>()) {
					DeserializeParticleComponent(component, entity);
				}
				else if (name == TypeName<SkyLightComponent>()) {
					DeserializeSkyLightComponent(component, entity);
				}
				else if (name == TypeName<DirectionalLightComponent>()) {
					DeserializeDirectionalLightComponent(component, entity);
				}
				else if (name == TypeName<PointLightComponent>()) {
					DeserializePointLightComponent(component, entity);
				}
				else if (name == TypeName<SpotLightComponent>()) {
					DeserializeSpotLightComponent(component, entity);
				}
				else if (name == TypeName<SkyBoxComponent>()) {
					DeserializeSkyBoxComponent(component, entity);
				}
				else if (name == TypeName<DecalComponent>()) {
					DeserializeDecalComponent(component, entity);
				}
				else if (name == TypeName<LandscapeComponent>()) {
					DeserializeLandscapeComponent(component, entity);
				}
				else if (name == TypeName<AnimatorComponent>()) {
					DeserializeAnimatorComponent(component, entity);
				}
				else if (name == TypeName<CanvasComponent>()) {
					DeserializeCanvasComponent(component, entity);
				}
				else if (name == TypeName<CanvasScalerComponent>()) {
					DeserializeCanvasScalerComponent(component, entity);
				}
				else if (name == TypeName<RectLayoutComponent>()) {
					DeserializeRectLayoutComponent(component, entity);
				}
				else if (name == TypeName<ImageComponent>()) {
					DeserializeImageComponent(component, entity);
				}
				else if (name == TypeName<MonoScriptComponent>()) {
					DeserializeMonoScriptComponent(component, entity);
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