#include "pch.h"
#include "LandscapeSystem.h"
#include "Scene.h"
#include "Components.h"
#include "Graphics/GraphicsFunc.h"
#include "Image/Image.h"
#include "AssetManager.h"
#include "Assets.h"
#include "RenderSystem.h"

namespace flaw {
	LandscapeSystem::LandscapeSystem(Scene& scene)
		: _scene(scene)
	{
		_landscapeShader = Graphics::CreateGraphicsShader("Resources/Shaders/landscape.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Hull | ShaderCompileFlag::Domain | ShaderCompileFlag::Pixel);
		_landscapeShader->AddInputElement<float>("POSITION", 3);
		_landscapeShader->AddInputElement<float>("TEXCOORD", 2);
		_landscapeShader->AddInputElement<float>("TANGENT", 3);
		_landscapeShader->AddInputElement<float>("NORMAL", 3);
		_landscapeShader->AddInputElement<float>("BINORMAL", 3);
		_landscapeShader->AddInputElement<int>("BONEINDICES", 4);
		_landscapeShader->AddInputElement<float>("BONEWEIGHTS", 4);
		_landscapeShader->CreateInputLayout();
	}

	void LandscapeSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		auto& landscapeComp = registry.get<LandScaperComponent>(entity);

		Landscape landscape;

		landscape.material = CreateRef<Material>();

		landscape.material->shader = _landscapeShader;
		landscape.material->renderMode = RenderMode::Opaque;
		landscape.material->cullMode = CullMode::Back;
		landscape.material->depthTest = DepthTest::Less;
		landscape.material->depthWrite = true;

		_landscapes[(uint32_t)entity] = landscape;

		landscapeComp.dirty = true;
	}

	Ref<Mesh> LandscapeSystem::CreateLandscapeMesh(uint32_t tilingX, uint32_t tilingY) {
		std::vector<Vertex3D> vertices;
		std::vector<uint32_t> indices;

		GenerateQuad(
			[&vertices](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) {
				Vertex3D vertex;
				vertex.position = vec3(pos.x, pos.z, pos.y);
				vertex.texcoord = uv;
				vertex.normal = vec3(normal.x, -normal.z, normal.y);
				vertex.tangent = vec3(tangent.x, -tangent.z, tangent.y);
				vertex.binormal = vec3(binormal.x, binormal.z, binormal.y);
				vertices.push_back(vertex);
			},
			indices,
			tilingX,
			tilingY
		);

		return CreateRef<Mesh>(PrimitiveTopology::ControlPoint3_PatchList, vertices, indices);
	}

	void LandscapeSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		_landscapes.erase((uint32_t)entity);
	}

	void LandscapeSystem::Update() {
		for (auto&& [entity, landscapeComp] : _scene.GetRegistry().view<LandScaperComponent>().each()) {
			if (!landscapeComp.dirty) {
				continue;
			}

			auto& landscape = _landscapes[(uint32_t)entity];

			landscape.mesh = CreateLandscapeMesh(landscapeComp.tilingX, landscapeComp.tilingY);

			auto tex2DAsset = AssetManager::GetAsset<Texture2DAsset>(landscapeComp.heightMap);
			if (tex2DAsset) {
				landscape.material->heightTexture = tex2DAsset->GetTexture();
			}
			else {
				landscape.material->heightTexture = nullptr;
			}

			auto tex2DArrayAsset = AssetManager::GetAsset<Texture2DArrayAsset>(landscapeComp.albedoTexture2DArray);
			if (tex2DArrayAsset) {
				landscape.material->textureArrays[0] = tex2DArrayAsset->GetTexture();
			}
			else {
				landscape.material->textureArrays[0] = nullptr;
			}

			landscape.material->intConstants[TilingXIndex] = landscapeComp.tilingX;
			landscape.material->intConstants[TilingYIndex] = landscapeComp.tilingY;
			landscape.material->vec2Constants[0] = vec2(0, landscapeComp.lodLevelMax);
			landscape.material->vec2Constants[1] = landscapeComp.lodDistanceRange;

			landscapeComp.dirty = false;
		}
	}

	void LandscapeSystem::GatherRenderable(CameraRenderStage& stage) {
		for (auto&& [entity, transComp, landscapeComp] : _scene.GetRegistry().view<TransformComponent, LandScaperComponent>().each()) {
			auto& landscape = _landscapes[(uint32_t)entity];

			const MeshBoundingSphere& boundingSphere = landscape.mesh->GetBoundingSphere();

			if (stage.frustum.TestInside(boundingSphere.center, boundingSphere.radius, transComp.worldTransform)) {
				stage.renderQueue.Push(landscape.mesh, transComp.worldTransform, landscape.material);
			}
		}
	}
}