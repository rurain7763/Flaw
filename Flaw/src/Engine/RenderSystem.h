#pragma once

#include "Core.h"
#include "Graphics.h"
#include "RenderQueue.h"

#include <map>
#include <vector>

namespace flaw {
	class Scene;

	struct CameraRenderStage {
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
		void CreateBatchedBuffers();
		void CreateConstantBuffers();
		void CreateStructuredBuffers();
		void CreatePipeline();

		void GatherLights();
		void GatherDecals();
		void GatherCameraStages(std::map<uint32_t, Camera>& cameras);

		void RenderGeometry(CameraRenderStage& stage);
		void RenderDecal(CameraRenderStage& stage);
		void RenderDefferdLighting(CameraRenderStage& stage);
		void RenderSkyBox(CameraRenderStage& stage);
		void RenderTransparent(CameraRenderStage& stage);
		void FinalizeRender(CameraRenderStage& stage);

	private:
		constexpr static uint32_t MaxBatchVertexCount = 10000;
		constexpr static uint32_t MaxBatchIndexCount = 30000;
		constexpr static uint32_t MaxBatchTransformCount = 100;
		constexpr static uint32_t MaxDecalCount = 1000;
		constexpr static uint32_t MaxDirectionalLights = 10;
		constexpr static uint32_t MaxPointLights = 10;
		constexpr static uint32_t MaxSpotLights = 10;

		Scene& _scene;

		enum RenderPassRenderTargetNumber {
			GeometryPosition = 0,
			GeometryNormal,
			GeometryAlbedo,
			GeometryEmissive,
			GeometryRTCount,

			DecalAlbedo = 0,
			DecalRTCount,

			LightingDiffuse = 0,
			LightingSpecular,
			LightingRTCount
		};

		Ref<GraphicsRenderPass> _geometryPass;
		Ref<GraphicsRenderPass> _decalPass;
		Ref<GraphicsRenderPass> _lightingPass;

		std::vector<CameraRenderStage> _renderStages;

		Ref<VertexBuffer> _batchedVertexBuffer;
		Ref<IndexBuffer> _batchedIndexBuffer;

		Ref<ConstantBuffer> _vpCB;
		Ref<ConstantBuffer> _globalCB;
		Ref<ConstantBuffer> _lightCB;
		Ref<ConstantBuffer> _materialCB;
		Ref<StructuredBuffer> _batchedTransformSB;
		Ref<StructuredBuffer> _directionalLightSB;
		Ref<StructuredBuffer> _pointLightSB;
		Ref<StructuredBuffer> _spotLightSB;
		Ref<StructuredBuffer> _decalSB;
		Ref<GraphicsPipeline> _pipeline;

		LightConstants _lightConstants;
		std::vector<DirectionalLight> _directionalLights;
		std::vector<PointLight> _pointLights;
		std::vector<SpotLight> _spotLights;

		std::vector<Decal> _decals;
		std::map<Ref<Texture2D>, uint32_t> _decalTextureIndexMap;
		std::vector<Ref<Texture2D>> _decalTextures;

		VPMatrices _vpMatrices;
		GlobalConstants _globalConstants;
		MaterialConstants _materialConstants;
	};
}