#pragma once

#include "Core.h"
#include "Graphics.h"
#include "RenderQueue.h"

#include <map>
#include <vector>

namespace flaw {
	class Scene;

	struct CameraRenderStage {
		vec3 cameraPosition;
		mat4 view;
		mat4 projection;
		RenderQueue renderQueue;
	};

	class RenderSystem {
	public:
		RenderSystem(Scene& scene);

		void Update();
		void Update(Camera& camera);

		void Render();

	private:
		void CreateRenderPasses();
		void CreateConstantBuffers();
		void CreateStructuredBuffers();

		void GatherLights();
		void GatherDecals();
		void GatherCameraStages(std::map<uint32_t, Camera>& cameras);

		void UpdateMateraialConstants(GraphicsCommandQueue& cmdQueue, const Ref<Material>& material);

		void RenderGeometry(CameraRenderStage& stage);
		void RenderDecal(CameraRenderStage& stage);
		void RenderDefferdLighting(CameraRenderStage& stage);
		void RenderTransparent(CameraRenderStage& stage);
		void FinalizeRender(CameraRenderStage& stage);

	private:
		constexpr static uint32_t MaxBatchVertexCount = 10000;
		constexpr static uint32_t MaxBatchIndexCount = 30000;
		constexpr static uint32_t MaxBatchTransformCount = 100;
		constexpr static uint32_t MaxDecalCount = 1000;
		constexpr static uint32_t MaxPointLights = 10;
		constexpr static uint32_t MaxSpotLights = 10;

		Scene& _scene;

		enum RenderPassRenderTargetNumber {
			GeometryPosition = 0,
			GeometryNormal,
			GeometryAlbedo,
			GeometryMaterial,
			GeometryEmissive,
			GeometryRTCount,

			DecalAlbedo = 0,
			DecalRTCount,

			LightingRadiance = 0,
			LightingShadow,
			LightingRTCount
		};

		Ref<GraphicsRenderPass> _geometryPass;
		Ref<GraphicsRenderPass> _decalPass;
		Ref<GraphicsRenderPass> _lightingPass;

		std::vector<CameraRenderStage> _renderStages;

		Ref<ConstantBuffer> _vpCB;
		Ref<ConstantBuffer> _globalCB;
		Ref<ConstantBuffer> _lightCB;
		Ref<ConstantBuffer> _materialCB;
		Ref<StructuredBuffer> _batchedTransformSB;

		struct DirectionalLightUniforms {
			mat4 view;
			mat4 projection;

			vec4 lightColor;
			vec4 lightDirection;
			float lightIntensity;

			float padding[3];
		};

		struct SpotLightUniforms {
			mat4 view;
			mat4 projection;

			vec4 lightColor;
			vec4 lightPosition;
			vec4 lightDirection;
			float lightIntensity;
			float lightRange;
			float innerAngle;
			float outerAngle;
		};

		struct PointLightUniforms {
			vec4 lightColor;
			vec4 lightPosition;
			float lightIntensity;
			float lightRange;

			float padding[2];
		};

		Ref<ConstantBuffer> _directionalLightUniformCB;
		Ref<ConstantBuffer> _spotLightUniformCB;
		Ref<ConstantBuffer> _pointLightUniformCB;
		Ref<StructuredBuffer> _decalSB;

		LightConstants _lightConstants;

		std::vector<Decal> _decals;
		std::map<Ref<Texture2D>, uint32_t> _decalTextureIndexMap;
		std::vector<Ref<Texture2D>> _decalTextures;

		CameraConstants _cameraConstansCB;
		GlobalConstants _globalConstants;
		MaterialConstants _materialConstants;
	};
}