#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Skeleton.h"
#include "ECS/ECS.h"
#include "Graphics.h"

namespace flaw {
	class Application;
	class Scene;

	struct SkeletonUniforms {
		std::vector<mat4> bindingPoseBoneMatrices;
		std::vector<mat4> bindingPoseSkinMatrices;

		Ref<StructuredBuffer> bindingPoseSkinMatricesSB;
	};

	struct SkeletonSocketAttachment {
		std::string socketName;
		entt::entity entity;
	};

	struct SkeletalData {
		std::vector<SkeletonSocketAttachment> socketAttachments;
	};

	class SkeletalSystem {
	public:
		SkeletalSystem(Application& app, Scene& scene);
		~SkeletalSystem();

		void Update();

		void AttachEntityToSocket(entt::entity entity, entt::entity targetEntity, const std::string& socketName);

		SkeletonUniforms& GetSkeletonUniforms(Ref<Skeleton> skeleton);
		SkeletalData& GetSkeletalData(entt::entity entity);

	private:
		void RegisterEntity(entt::registry& registry, entt::entity entity);
		void UnregisterEntity(entt::registry& registry, entt::entity entity);

	private:
		Application& _app;
		Scene& _scene;

		std::unordered_map<Ref<Skeleton>, SkeletonUniforms> _skeletonUniforms;
		std::unordered_map<entt::entity, SkeletalData> _skeletonDatas;
	};
}