#include "pch.h"
#include "SkeletalSystem.h"
#include "Components.h"
#include "Scene.h"
#include "Application.h"
#include "Assets.h"
#include "AssetManager.h"
#include "AnimationSystem.h"

namespace flaw {
	SkeletalSystem::SkeletalSystem(Application& app, Scene& scene)
		: _app(app)
		, _scene(scene)
	{
		auto& registry = _scene.GetRegistry();
		registry.on_construct<SkeletalMeshComponent>().connect<&SkeletalSystem::RegisterEntity>(*this);
		registry.on_destroy<SkeletalMeshComponent>().connect<&SkeletalSystem::UnregisterEntity>(*this);
	}

	SkeletalSystem::~SkeletalSystem() {
		auto& registry = _scene.GetRegistry();
		registry.on_construct<SkeletalMeshComponent>().disconnect<&SkeletalSystem::RegisterEntity>(*this);
		registry.on_destroy<SkeletalMeshComponent>().disconnect<&SkeletalSystem::UnregisterEntity>(*this);
	}

	void SkeletalSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		auto& skeletalMeshComp = registry.get<SkeletalMeshComponent>(entity);
		_skeletonDatas[entity] = SkeletalData();
	}

	void SkeletalSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		_skeletonDatas.erase(entity);
	}

	void SkeletalSystem::Update() {
		auto& registry = _scene.GetRegistry();
		auto& animationSys = _scene.GetAnimationSystem();

		for (auto&& [entity, transComp, skeletalMeshComp] : registry.view<TransformComponent, SkeletalMeshComponent>().each()) {
			auto it = _skeletonDatas.find(entity);
			if (it == _skeletonDatas.end()) {
				continue;
			}

			auto& skeletonData = it->second;

			auto skeletonAsset = AssetManager::GetAsset<SkeletonAsset>(skeletalMeshComp.skeleton);
			if (!skeletonAsset) {
				continue; // Skeleton asset not found
			}

			auto skeleton = skeletonAsset->GetSkeleton();
			
			for (auto attIt = skeletonData.socketAttachments.begin(); attIt != skeletonData.socketAttachments.end(); ) {
				auto& attachment = *attIt;

				if (!skeleton->HasSocket(attachment.socketName)) {
					attIt = skeletonData.socketAttachments.erase(attIt);
					continue;
				}

				auto& socketNode = skeleton->GetSocket(attachment.socketName);
				Entity targetEntity(attachment.entity, &_scene);
				if (!targetEntity) {
					attIt = skeletonData.socketAttachments.erase(attIt);
					continue;
				}

				auto& targetEnttTransComp = targetEntity.GetComponent<TransformComponent>();
				mat4 globalTransform = mat4(1.0f);
				if (animationSys.HasAnimatorJobContext(entity)) {
					auto& animatorContext = animationSys.GetAnimatorJobContext(entity);
					auto& animatedPoseBoneMatrices = *animatorContext.frontAnimatedBoneMatrices;

					globalTransform = transComp.worldTransform * animatedPoseBoneMatrices[socketNode.boneIndex] * socketNode.localTransform;
				}
				else {
					auto& skeletonUniforms = GetSkeletonUniforms(skeleton);
					auto& bindingPoseBoneMatrices = skeletonUniforms.bindingPoseBoneMatrices;

					globalTransform = transComp.worldTransform * bindingPoseBoneMatrices[socketNode.boneIndex] * socketNode.localTransform;
				}

				_scene.UpdateTransformImmediate(targetEntity);

				mat4 parentWorldMatrix = targetEntity.HasParent() ? targetEntity.GetParent().GetComponent<TransformComponent>().worldTransform : mat4(1.0f);
				mat4 localMatrix = glm::inverse(parentWorldMatrix) * globalTransform;
				targetEnttTransComp.position = ExtractPosition(localMatrix);
				targetEnttTransComp.rotation = ExtractRotation(localMatrix);
				targetEnttTransComp.dirty = true;

				++attIt;
			}
		}
	}

	void SkeletalSystem::AttachEntityToSocket(entt::entity entity, entt::entity targetEntity, const std::string& socketName) {
		auto it = _skeletonDatas.find(entity);
		if (it == _skeletonDatas.end()) {
			Log::Error("SkeletalSystem: Entity does not have a skeleton data!");
			return;
		}

		auto& skeletonData = it->second;
		skeletonData.socketAttachments.push_back({ socketName, targetEntity });
	}

	SkeletonUniforms& SkeletalSystem::GetSkeletonUniforms(Ref<Skeleton> skeleton) {
		auto it = _skeletonUniforms.find(skeleton);
		if (it != _skeletonUniforms.end()) {
			return it->second;
		}

		SkeletonUniforms& uniforms = _skeletonUniforms[skeleton];

		skeleton->GetBindingPoseSkinMatrices(uniforms.bindingPoseSkinMatrices);

		StructuredBuffer::Descriptor desc = {};
		desc.elmSize = sizeof(mat4);
		desc.count = uniforms.bindingPoseSkinMatrices.size();
		desc.bindFlags = BindFlag::ShaderResource;
		desc.initialData = uniforms.bindingPoseSkinMatrices.data();

		uniforms.bindingPoseSkinMatricesSB = Graphics::CreateStructuredBuffer(desc);

		return uniforms;
	}

	SkeletalData& SkeletalSystem::GetSkeletalData(entt::entity entity) {
		auto it = _skeletonDatas.find(entity);
		if (it != _skeletonDatas.end()) {
			return it->second;
		}

		throw std::runtime_error("SkeletalSystem: Entity does not have skeleton data!");
	}
}