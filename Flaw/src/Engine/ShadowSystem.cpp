#include "pch.h"
#include "ShadowSystem.h"
#include "Scene.h"
#include "AssetManager.h"
#include "Assets.h"
#include "AnimationSystem.h"
#include "SkeletalSystem.h"
#include "Graphics/GraphicsFunc.h"

namespace flaw {
	ShadowSystem::ShadowSystem(Scene& scene)
		: _scene(scene)
	{
		Ref<GraphicsShader> shadowMapShader = Graphics::CreateGraphicsShader("Resources/Shaders/shadowmap_skeletal.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Geometry | ShaderCompileFlag::Pixel);
		shadowMapShader->AddInputElement<float>("POSITION", 3);
		shadowMapShader->AddInputElement<float>("TEXCOORD", 2);
		shadowMapShader->AddInputElement<float>("TANGENT", 3);
		shadowMapShader->AddInputElement<float>("NORMAL", 3);
		shadowMapShader->AddInputElement<float>("BINORMAL", 3);
		shadowMapShader->AddInputElement<int32_t>("BONEINDICES", 4);
		shadowMapShader->AddInputElement<float>("BONEWEIGHTS", 4);
		shadowMapShader->CreateInputLayout();

		_shadowMapSkeletalMaterial = CreateRef<Material>();
		_shadowMapSkeletalMaterial->shader = shadowMapShader;
		_shadowMapSkeletalMaterial->renderMode = RenderMode::Opaque;
		_shadowMapSkeletalMaterial->cullMode = CullMode::Front;
		_shadowMapSkeletalMaterial->depthTest = DepthTest::Less;
		_shadowMapSkeletalMaterial->depthWrite = true;

		shadowMapShader = Graphics::CreateGraphicsShader("Resources/Shaders/shadowmap.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Geometry | ShaderCompileFlag::Pixel);
		shadowMapShader->AddInputElement<float>("POSITION", 3);
		shadowMapShader->AddInputElement<float>("TEXCOORD", 2);
		shadowMapShader->AddInputElement<float>("TANGENT", 3);
		shadowMapShader->AddInputElement<float>("NORMAL", 3);
		shadowMapShader->AddInputElement<float>("BINORMAL", 3);
		shadowMapShader->CreateInputLayout();

		_shadowMapStaticMaterial = CreateRef<Material>();
		_shadowMapStaticMaterial->shader = shadowMapShader;
		_shadowMapStaticMaterial->renderMode = RenderMode::Opaque;
		_shadowMapStaticMaterial->cullMode = CullMode::Front;
		_shadowMapStaticMaterial->depthTest = DepthTest::Less;
		_shadowMapStaticMaterial->depthWrite = true;

		_shadowUniformsCB = Graphics::CreateConstantBuffer(sizeof(ShadowUniforms));

		StructuredBuffer::Descriptor sbDesc = {};
		sbDesc.elmSize = sizeof(LightVPMatrix);
		sbDesc.count = MaxLightVPCount;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		sbDesc.accessFlags = AccessFlag::Write;

		_lightVPMatricesSB = Graphics::CreateStructuredBuffer(sbDesc);

		auto& registry = _scene.GetRegistry();
		registry.on_construct<DirectionalLightComponent>().connect<&ShadowSystem::RegisterEntity<DirectionalLightComponent>>(*this);
		registry.on_destroy<DirectionalLightComponent>().connect<&ShadowSystem::UnregisterEntity<DirectionalLightComponent>>(*this);
		registry.on_construct<PointLightComponent>().connect<&ShadowSystem::RegisterEntity<PointLightComponent>>(*this);
		registry.on_destroy<PointLightComponent>().connect<&ShadowSystem::UnregisterEntity<PointLightComponent>>(*this);
		registry.on_construct<SpotLightComponent>().connect<&ShadowSystem::RegisterEntity<SpotLightComponent>>(*this);
		registry.on_destroy<SpotLightComponent>().connect<&ShadowSystem::UnregisterEntity<SpotLightComponent>>(*this);
	}

	ShadowSystem::~ShadowSystem() {
		auto& registry = _scene.GetRegistry();
		registry.on_construct<DirectionalLightComponent>().disconnect<&ShadowSystem::RegisterEntity<DirectionalLightComponent>>(*this);
		registry.on_destroy<DirectionalLightComponent>().disconnect<&ShadowSystem::UnregisterEntity<DirectionalLightComponent>>(*this);
		registry.on_construct<PointLightComponent>().disconnect<&ShadowSystem::RegisterEntity<PointLightComponent>>(*this);
		registry.on_destroy<PointLightComponent>().disconnect<&ShadowSystem::UnregisterEntity<PointLightComponent>>(*this);
		registry.on_construct<SpotLightComponent>().disconnect<&ShadowSystem::RegisterEntity<SpotLightComponent>>(*this);
		registry.on_destroy<SpotLightComponent>().disconnect<&ShadowSystem::UnregisterEntity<SpotLightComponent>>(*this);
	}

	Ref<GraphicsRenderPass> ShadowSystem::CreateDirectionalLightShadowMapRenderPass(uint32_t width, uint32_t height) {
		Texture2D::Descriptor texDesc = {};
		texDesc.width = width;
		texDesc.height = height;
		texDesc.usage = UsageFlag::Static;
		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::R32F;

		GraphicsRenderPass::Descriptor shadowMapDesc = {};
		shadowMapDesc.renderTargets.resize(1);
		shadowMapDesc.renderTargets[0].blendMode = BlendMode::Disabled;
		shadowMapDesc.renderTargets[0].texture = Graphics::CreateTexture2D(texDesc);
		shadowMapDesc.renderTargets[0].viewportX = 0;
		shadowMapDesc.renderTargets[0].viewportY = 0;
		shadowMapDesc.renderTargets[0].viewportWidth = width;
		shadowMapDesc.renderTargets[0].viewportHeight = height;
		shadowMapDesc.renderTargets[0].clearValue = { 1.0f, 1.0f, 1.0f, 0.0f };

		texDesc.bindFlags = BindFlag::DepthStencil;
		texDesc.format = PixelFormat::D24S8_UINT;
		shadowMapDesc.depthStencil.texture = Graphics::CreateTexture2D(texDesc);

		return Graphics::CreateRenderPass(shadowMapDesc);
	}

	Ref<GraphicsRenderPass> ShadowSystem::CreateSpotLightShadowMapRenderPass() {
		Texture2D::Descriptor texDesc = {};
		texDesc.width = ShadowMapSize;
		texDesc.height = ShadowMapSize;
		texDesc.usage = UsageFlag::Static;
		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::R32F;

		GraphicsRenderPass::Descriptor shadowMapDesc = {};
		shadowMapDesc.renderTargets.resize(1);
		shadowMapDesc.renderTargets[0].blendMode = BlendMode::Disabled;
		shadowMapDesc.renderTargets[0].texture = Graphics::CreateTexture2D(texDesc);
		shadowMapDesc.renderTargets[0].viewportX = 0;
		shadowMapDesc.renderTargets[0].viewportY = 0;
		shadowMapDesc.renderTargets[0].viewportWidth = ShadowMapSize;
		shadowMapDesc.renderTargets[0].viewportHeight = ShadowMapSize;
		shadowMapDesc.renderTargets[0].clearValue = { 1.0f, 1.0f, 1.0f, 0.0f };

		texDesc.bindFlags = BindFlag::DepthStencil;
		texDesc.format = PixelFormat::D24S8_UINT;
		shadowMapDesc.depthStencil.texture = Graphics::CreateTexture2D(texDesc);

		return Graphics::CreateRenderPass(shadowMapDesc);
	}

	Ref<GraphicsRenderPass> ShadowSystem::CreatePointLightShadowMapRenderPass() {
		TextureCube::Descriptor texDesc = {};
		texDesc.width = ShadowMapSize;
		texDesc.height = ShadowMapSize;
		texDesc.usage = UsageFlag::Static;
		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::R32F;

		GraphicsRenderPass::Descriptor shadowMapDesc = {};
		shadowMapDesc.renderTargets.resize(1);
		shadowMapDesc.renderTargets[0].blendMode = BlendMode::Disabled;
		shadowMapDesc.renderTargets[0].texture = Graphics::CreateTextureCube(texDesc);
		shadowMapDesc.renderTargets[0].viewportX = 0;
		shadowMapDesc.renderTargets[0].viewportY = 0;
		shadowMapDesc.renderTargets[0].viewportWidth = ShadowMapSize;
		shadowMapDesc.renderTargets[0].viewportHeight = ShadowMapSize;
		shadowMapDesc.renderTargets[0].clearValue = { 1.0f, 1.0f, 1.0f, 0.0f };

		texDesc.bindFlags = BindFlag::DepthStencil;
		texDesc.format = PixelFormat::D24S8_UINT;
		shadowMapDesc.depthStencil.texture = Graphics::CreateTextureCube(texDesc);

		return Graphics::CreateRenderPass(shadowMapDesc);
	}

	void ShadowSystem::Update() {
		auto& registry = _scene.GetRegistry();
		auto& animationSys = _scene.GetAnimationSystem();
		auto& skeletalSys = _scene.GetSkeletalSystem();

		for (auto&& [entity, transComp, lightComp] : registry.view<TransformComponent, DirectionalLightComponent>().each()) {
			auto& shadowMap = _directionalShadowMaps[entity];

			// not calculating here, because we need to calculate tight bounding box for directional light
			shadowMap.lightDirection = transComp.GetWorldFront();

			for (int32_t i = 0; i < CascadeShadowCount; ++i) {
				shadowMap.renderPasses[i]->ClearAllRenderTargets();
				shadowMap.renderPasses[i]->ClearDepthStencil();
			}
		}

		for (auto&& [entity, transform, lightComp] : registry.view<TransformComponent, SpotLightComponent>().each()) {
			auto& shadowMap = _spotLightShadowMaps[entity];

			shadowMap.lightVPMatrix.view = LookAt(transform.GetWorldPosition(), transform.GetWorldPosition() + transform.GetWorldFront(), Up);
			shadowMap.lightVPMatrix.projection = Perspective(lightComp.outer * 2.0, 1.0f, 0.1f, lightComp.range);

			shadowMap.renderPass->ClearAllRenderTargets();
			shadowMap.renderPass->ClearDepthStencil();
		}

		for (auto&& [entity, transComp, lightComp] : registry.view<TransformComponent, PointLightComponent>().each()) {
			auto& shadowMap = _pointLightShadowMaps[entity];

			const vec3 faceDirections[6] = {
				Right, -Right, Up, -Up, Forward, -Forward
			};

			const vec3 upDirections[6] = {
				Up, Up, -Forward, Forward, Up, Up
			};

			for (int i = 0; i < 6; ++i) {
				shadowMap.lightVPMatrices[i].view = LookAt(transComp.GetWorldPosition(), transComp.GetWorldPosition() + faceDirections[i], upDirections[i]);
				shadowMap.lightVPMatrices[i].projection = Perspective(glm::half_pi<float>(), 1.0f, 0.1f, lightComp.range);
			}

			shadowMap.renderPass->ClearAllRenderTargets();
			shadowMap.renderPass->ClearDepthStencil();
		}

		_shadowMapRenderQueue.Open();

		for (auto&& [entity, transform, staticMeshComp] : registry.view<TransformComponent, StaticMeshComponent>().each()) {
			if (staticMeshComp.castShadow) {
				auto& staticMeshAsset = AssetManager::GetAsset<StaticMeshAsset>(staticMeshComp.mesh);
				if (staticMeshAsset == nullptr) {
					continue;
				}

				_shadowMapRenderQueue.Push(staticMeshAsset->GetMesh(), transform.worldTransform, _shadowMapStaticMaterial);
			}
		}

		for (auto&& [entity, transform, skeletalMeshComp] : registry.view<TransformComponent, SkeletalMeshComponent>().each()) {
			if (!skeletalMeshComp.castShadow) {
				continue;
			}

			auto meshAsset = AssetManager::GetAsset<SkeletalMeshAsset>(skeletalMeshComp.mesh);
			if (meshAsset == nullptr) {
				continue;
			}

			Ref<Mesh> mesh = meshAsset->GetMesh();

			Ref<StructuredBuffer> boneMatricesSB;

			auto skeletonAsset = AssetManager::GetAsset<SkeletonAsset>(meshAsset->GetSkeletonHandle());
			if (!skeletonAsset) {
				continue;
			}

			if (animationSys.HasAnimatorJobContext(entity)) {
				boneMatricesSB = animationSys.GetAnimatorJobContext(entity).animatedSkinMatricesSB;
			}
			else {
				auto& skeletonUniforms = skeletalSys.GetSkeletonUniforms(skeletonAsset->GetSkeleton());
				boneMatricesSB = skeletonUniforms.bindingPoseSkinMatricesSB;
			}

			if (!boneMatricesSB) {
				continue;
			}

			_shadowMapRenderQueue.Push(mesh, transform.worldTransform, _shadowMapSkeletalMaterial, boneMatricesSB);
		}

		_shadowMapRenderQueue.Close();
	}

#if false // linear split cascade
	std::vector<Frustum::Corners> ShadowSystem::GetCascadeFrustumCorners(const Frustum& frustum) {
		Frustum::Corners worldSpaceCorners = frustum.GetCorners();
		vec3 tln2tlfDir = worldSpaceCorners.topLeftFar - worldSpaceCorners.topLeftNear;
		float dist = glm::length(tln2tlfDir);

		vec3 directions[4] = {
			tln2tlfDir / dist,
			(worldSpaceCorners.topRightFar - worldSpaceCorners.topRightNear) / dist,
			(worldSpaceCorners.bottomRightFar - worldSpaceCorners.bottomRightNear) / dist,
			(worldSpaceCorners.bottomLeftFar - worldSpaceCorners.bottomLeftNear) / dist
		};

		float stepDist = dist / (float)CascadeShadowCount;

		std::vector<Frustum::Corners> cascadeCornersList(CascadeShadowCount);

		for (int32_t i = 0; i < 4; i++) {
			auto& cascadeCorners = cascadeCornersList[0];

			cascadeCorners.data[i] = worldSpaceCorners.data[i];
			cascadeCorners.data[i + 4] = worldSpaceCorners.data[i] + directions[i] * stepDist;
		}

		for (int32_t i = 1; i < CascadeShadowCount; ++i) {
			const auto& prevCascadeCorners = cascadeCornersList[i - 1];
			auto& cascadeCorners = cascadeCornersList[i];

			for (int32_t j = 0; j < 4; j++) {
				cascadeCorners.data[j] = prevCascadeCorners.data[j + 4];
				cascadeCorners.data[j + 4] = prevCascadeCorners.data[j + 4] + directions[j] * stepDist;
			}
		}

		return cascadeCornersList;
	}
#else // logarithmic split cascade
	std::vector<Frustum::Corners> ShadowSystem::GetCascadeFrustumCorners(const Frustum& frustum) {
		Frustum::Corners worldSpaceCorners = frustum.GetCorners();

		vec3 viewDirTL = worldSpaceCorners.topLeftFar - worldSpaceCorners.topLeftNear;
		vec3 viewDirTR = worldSpaceCorners.topRightFar - worldSpaceCorners.topRightNear;
		vec3 viewDirBR = worldSpaceCorners.bottomRightFar - worldSpaceCorners.bottomRightNear;
		vec3 viewDirBL = worldSpaceCorners.bottomLeftFar - worldSpaceCorners.bottomLeftNear;

		float nearDist = glm::length(viewDirTL);
		float farDist = nearDist * CascadeShadowCount;

		float lambda = 0.95f;

		std::vector<float> cascadeSplits(CascadeShadowCount + 1);
		cascadeSplits[0] = 0.0f;
		cascadeSplits[CascadeShadowCount] = 1.0f;

		for (int i = 1; i < CascadeShadowCount; ++i) {
			float p = (float)i / (float)CascadeShadowCount;
			float logSplit = nearDist * std::pow(farDist / nearDist, p);
			float linearSplit = nearDist + (farDist - nearDist) * p;
			float splitDist = lambda * logSplit + (1.0f - lambda) * linearSplit;

			cascadeSplits[i] = (splitDist - nearDist) / (farDist - nearDist);
		}

		std::vector<Frustum::Corners> cascadeCornersList(CascadeShadowCount);

		for (int i = 0; i < CascadeShadowCount; ++i) {
			float startRatio = cascadeSplits[i];
			float endRatio = cascadeSplits[i + 1];

			Frustum::Corners& corners = cascadeCornersList[i];

			corners.topLeftNear = worldSpaceCorners.topLeftNear + viewDirTL * startRatio;
			corners.topRightNear = worldSpaceCorners.topRightNear + viewDirTR * startRatio;
			corners.bottomRightNear = worldSpaceCorners.bottomRightNear + viewDirBR * startRatio;
			corners.bottomLeftNear = worldSpaceCorners.bottomLeftNear + viewDirBL * startRatio;

			corners.topLeftFar = worldSpaceCorners.topLeftNear + viewDirTL * endRatio;
			corners.topRightFar = worldSpaceCorners.topRightNear + viewDirTR * endRatio;
			corners.bottomRightFar = worldSpaceCorners.bottomRightNear + viewDirBR * endRatio;
			corners.bottomLeftFar = worldSpaceCorners.bottomLeftNear + viewDirBL * endRatio;
		}

		return cascadeCornersList;
	}
#endif

	void ShadowSystem::CalcTightDirectionalLightMatrices(const Frustum::Corners& worldSpaceCorners, const vec3& lightDirection, mat4& outView, mat4& outProjection) {
		mat4 lightViewMatrix = LookAt(vec3(0.0), lightDirection, Up);

		// # calculate corners coordinates in light space and get min, max coord elements
		vec3 minCornerInLightView = vec3(std::numeric_limits<float>::max());
		vec3 maxCornerInLightView = vec3(std::numeric_limits<float>::lowest());
		for (int32_t i = 0; i < 8; ++i) {
			const vec3& worldSpaceCorner = worldSpaceCorners.data[i];
			const vec3 lightSpaceCorner = lightViewMatrix * vec4(worldSpaceCorner, 1.0f);

			minCornerInLightView = glm::min(minCornerInLightView, lightSpaceCorner);
			maxCornerInLightView = glm::max(maxCornerInLightView, lightSpaceCorner);
		}

		// # get center position of aabb for light position
		vec3 lightPosition = (minCornerInLightView + maxCornerInLightView) * 0.5f;
		lightPosition.z = minCornerInLightView.z;

		// # translate light position to world space
		mat4 invLightViewMatrix = glm::inverse(lightViewMatrix);
		lightPosition = invLightViewMatrix * vec4(lightPosition, 1.0f);

		// # calculate view matrix
		outView = LookAt(lightPosition, lightPosition + lightDirection, Up);

		// # get aabb coordinates in new light view space
		minCornerInLightView = vec3(std::numeric_limits<float>::max());
		maxCornerInLightView = vec3(std::numeric_limits<float>::lowest());
		for (int32_t i = 0; i < 8; ++i) {
			const vec3& worldSpaceCorner = worldSpaceCorners.data[i];
			vec3 lightSpaceCorner = outView * vec4(worldSpaceCorner, 1.0f);

			minCornerInLightView = glm::min(minCornerInLightView, lightSpaceCorner);
			maxCornerInLightView = glm::max(maxCornerInLightView, lightSpaceCorner);
		}

		// # calculate projection matrix
		outProjection = Orthographic(minCornerInLightView.x, maxCornerInLightView.x, minCornerInLightView.y, maxCornerInLightView.y, minCornerInLightView.z, maxCornerInLightView.z);
	}

	void ShadowSystem::Render(const vec3& cameraPos, const Frustum& cameraFrustum) {
		auto worldSpaceCornersArr = GetCascadeFrustumCorners(cameraFrustum);
		
		while (!_shadowMapRenderQueue.Empty()) {
			auto& entry = _shadowMapRenderQueue.Front();
			_shadowMapRenderQueue.Pop();

			for (auto& [entt, shadowMap] : _directionalShadowMaps) {
				for (int32_t i = 0; i < CascadeShadowCount; i++) {
					const auto& worldSpaceCorners = worldSpaceCornersArr[i];
					auto& vpMatrix = shadowMap.lightVPMatrices[i];

					CalcTightDirectionalLightMatrices(worldSpaceCorners, shadowMap.lightDirection, vpMatrix.view, vpMatrix.projection);
					
					vec3 p0 = glm::mix(worldSpaceCorners.topLeftFar, worldSpaceCorners.topRightFar, 0.5f);
					vec3 p1 = glm::mix(worldSpaceCorners.bottomLeftFar, worldSpaceCorners.bottomRightFar, 0.5f);
					vec3 center = glm::mix(p0, p1, 0.5f);
					
					shadowMap.cascadeDistances[i] = glm::length(center - cameraPos);

					shadowMap.renderPasses[i]->Bind(false, false);
					DrawRenderEntry(entry, &vpMatrix, 1);
					shadowMap.renderPasses[i]->Unbind();
				}
			}

			for (const auto& [entt, shadowMap] : _spotLightShadowMaps) {
				shadowMap.renderPass->Bind(false, false);
				DrawRenderEntry(entry, &shadowMap.lightVPMatrix, 1);
				shadowMap.renderPass->Unbind();
			}

			for (const auto& [entt, shadowMap] : _pointLightShadowMaps) {
				shadowMap.renderPass->Bind(false, false);
				DrawRenderEntry(entry, shadowMap.lightVPMatrices.data(), 6);
				shadowMap.renderPass->Unbind();
			}
		}
	}

	void ShadowSystem::DrawRenderEntry(const RenderEntry& entry, const LightVPMatrix* lightVPMatrices, int32_t lightVPMatrixCount) {
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();
		auto batchedTransformSB = Graphics::GetBatchedDataSB();
		
		pipeline->SetShader(entry.material->shader);
		pipeline->SetFillMode(FillMode::Solid);
		pipeline->SetCullMode(entry.material->cullMode);
		pipeline->SetDepthTest(entry.material->depthTest, entry.material->depthWrite);

		ShadowUniforms shadowUniforms = {};
		shadowUniforms.lightVPMatrixCount = lightVPMatrixCount;

		_shadowUniformsCB->Update(&shadowUniforms, sizeof(ShadowUniforms));
		_lightVPMatricesSB->Update(lightVPMatrices, lightVPMatrixCount * sizeof(LightVPMatrix));

		cmdQueue.SetPipeline(pipeline);
		cmdQueue.SetConstantBuffer(_shadowUniformsCB, 0);
		cmdQueue.SetStructuredBuffer(batchedTransformSB, 0);
		cmdQueue.SetStructuredBuffer(_lightVPMatricesSB, 1);

		cmdQueue.Execute();

		// instancing draw
		for (auto& obj : entry.instancingObjects) {
			auto& mesh = obj.mesh;
			auto& meshSegment = mesh->GetMeshSegementAt(obj.segmentIndex);

			batchedTransformSB->Update(obj.batchedDatas.data(), obj.batchedDatas.size() * sizeof(BatchedData));

			cmdQueue.SetPrimitiveTopology(meshSegment.topology);
			cmdQueue.SetVertexBuffer(mesh->GetGPUVertexBuffer());
			cmdQueue.DrawIndexedInstanced(mesh->GetGPUIndexBuffer(), meshSegment.indexCount, obj.instanceCount, meshSegment.indexStart, meshSegment.vertexStart);
			cmdQueue.Execute();
		}

		for (auto& obj : entry.skeletalInstancingObjects) {
			auto& mesh = obj.mesh;
			auto& meshSegment = mesh->GetMeshSegementAt(obj.segmentIndex);

			batchedTransformSB->Update(obj.batchedDatas.data(), obj.batchedDatas.size() * sizeof(BatchedData));

			cmdQueue.SetPrimitiveTopology(meshSegment.topology);
			cmdQueue.SetStructuredBuffer(obj.skeletonBoneMatrices, 2);
			cmdQueue.SetVertexBuffer(mesh->GetGPUVertexBuffer());
			cmdQueue.DrawIndexedInstanced(mesh->GetGPUIndexBuffer(), meshSegment.indexCount, obj.instanceCount, meshSegment.indexStart, meshSegment.vertexStart);
			cmdQueue.Execute();
		}
	}
}