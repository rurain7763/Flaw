#include "pch.h"
#include "LandscapeSystem.h"
#include "Scene.h"
#include "Components.h"
#include "Graphics/GraphicsFunc.h"
#include "Image/Image.h"

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
		_landscapeShader->CreateInputLayout();
	}

	void LandscapeSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);
		auto& landscapeComp = registry.get<LandScaperComponent>(entity);

		Landscape landscape;

		// TODO: test code
		landscape.material = CreateRef<Material>();

		landscape.material->shader = _landscapeShader;
		landscape.material->renderMode = RenderMode::Opaque;
		landscape.material->cullMode = CullMode::Back;
		landscape.material->depthTest = DepthTest::Less;
		landscape.material->depthWrite = true;

		Image albedo("Resources/HeightMap.jpg", 4);

		Texture2D::Descriptor desc = {};
		desc.width = albedo.Width();
		desc.height = albedo.Height();
		desc.format = PixelFormat::RGBA8;
		desc.data = albedo.Data().data();
		desc.usage = UsageFlag::Static;
		desc.bindFlags = BindFlag::ShaderResource;

		landscape.material->heightTexture = Graphics::CreateTexture2D(desc);
		landscape.material->intConstants[TilingXIndex] = landscapeComp.tilingX;
		landscape.material->intConstants[TilingYIndex] = landscapeComp.tilingY;
		landscape.material->floatConstants[TesselationFactorIndex] = landscapeComp.tesselationFactor;

		_landscapes[enttComp.uuid] = landscape;

		landscapeComp.dirty = true;
	}

	Ref<Mesh> LandscapeSystem::CreateLandscapeMesh(uint32_t tilingX, uint32_t tilingY) {
		Ref<Mesh> mesh = CreateRef<Mesh>();

		mesh->topology = PrimitiveTopology::ControlPoint3_PatchList;

		GenerateQuad([&mesh](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) {
				Vertex3D vertex;
				vertex.position = vec3(pos.x, pos.z, pos.y);
				vertex.texcoord = uv;
				vertex.normal = vec3(normal.x, -normal.z, normal.y);
				vertex.tangent = vec3(tangent.x, -tangent.z, tangent.y);
				vertex.binormal = vec3(binormal.x, binormal.z, binormal.y);
				mesh->vertices.push_back(vertex);
			},
			mesh->indices,
			tilingX,
			tilingY
		);

		std::vector<vec3> vertices;
		std::transform(mesh->vertices.begin(), mesh->vertices.end(), std::back_inserter(vertices), [](const Vertex3D& vertex) { return vertex.position; });
		CreateBoundingSphere(vertices, mesh->boundingSphereCenter, mesh->boundingSphereRadius);

		return mesh;
	}

	void LandscapeSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);
		if (_landscapes.find(enttComp.uuid) != _landscapes.end()) {
			_landscapes.erase(enttComp.uuid);
		}
	}

	void LandscapeSystem::Update() {
		for (auto&& [entity, enttComp, landscapeComp] : _scene.GetRegistry().view<EntityComponent, LandScaperComponent>().each()) {
			if (!landscapeComp.dirty) {
				continue;
			}

			auto& landscape = _landscapes[enttComp.uuid];

			landscape.mesh = CreateLandscapeMesh(landscapeComp.tilingX, landscapeComp.tilingY);
			landscape.material->intConstants[TilingXIndex] = landscapeComp.tilingX;
			landscape.material->intConstants[TilingYIndex] = landscapeComp.tilingY;
			landscape.material->floatConstants[TesselationFactorIndex] = landscapeComp.tesselationFactor;

			landscapeComp.dirty = false;
		}
	}

	void LandscapeSystem::Render(const Camera& camera, RenderQueue& renderQueue) {
		// submit landscape
		for (auto&& [entity, enttComp, transComp, landscapeComp] : _scene.GetRegistry().view<EntityComponent, TransformComponent, LandScaperComponent>().each()) {
			auto& landscape = _landscapes[enttComp.uuid];

			if (camera.isPerspective && camera.TestInFrustum(landscape.mesh->boundingSphereCenter, landscape.mesh->boundingSphereRadius, transComp.worldTransform)) {
				renderQueue.Push(landscape.mesh, transComp.worldTransform, landscape.material);
			}
		}
	}
}