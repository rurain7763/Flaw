#include "pch.h"
#include "AssetManager.h"
#include "Application.h"
#include "Log/Log.h"
#include "Image/Image.h"
#include "Graphics/Texture.h"
#include "Graphics.h"
#include "Graphics/GraphicsFunc.h"
#include "Serialization.h"

namespace flaw {
	static std::unordered_map<std::string, AssetHandle> g_assetKeyMap;
	static std::unordered_map<AssetHandle, Ref<Asset>> g_registeredAssets;

	void AssetManager::Init() {
		RegisterDefaultGraphicsShaders();
		RegisterDefaultMaterials();
		RegisterDefaultStaticMeshs();
	}

	void AssetManager::RegisterDefaultGraphicsShaders() {
		AssetHandle handle = g_registeredAssets.size();
		RegisterAsset(handle, CreateRef<GraphicsShaderAsset>([](GraphicsShaderAsset::Descriptor& desc) {
			desc.shaderPath = "Resources/Shaders/std3d_geometry.fx";
			desc.shaderCompileFlags = ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel;
			desc.inputElements.push_back({ "POSITION", GraphicsShader::InputElement::ElementType::Float, 3, false });
			desc.inputElements.push_back({ "TEXCOORD", GraphicsShader::InputElement::ElementType::Float, 2, false });
			desc.inputElements.push_back({ "TANGENT", GraphicsShader::InputElement::ElementType::Float, 3, false });
			desc.inputElements.push_back({ "NORMAL", GraphicsShader::InputElement::ElementType::Float, 3, false });
			desc.inputElements.push_back({ "BINORMAL", GraphicsShader::InputElement::ElementType::Float, 3, false });
		}));
		RegisterKey("std3d_geometry_static", handle);

		handle = g_registeredAssets.size();
		RegisterAsset(handle, CreateRef<GraphicsShaderAsset>([](GraphicsShaderAsset::Descriptor& desc) {
			desc.shaderPath = "Resources/Shaders/std3d_geometry_skeletal.fx";
			desc.shaderCompileFlags = ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel;
			desc.inputElements.push_back({ "POSITION", GraphicsShader::InputElement::ElementType::Float, 3, false });
			desc.inputElements.push_back({ "TEXCOORD", GraphicsShader::InputElement::ElementType::Float, 2, false });
			desc.inputElements.push_back({ "TANGENT", GraphicsShader::InputElement::ElementType::Float, 3, false });
			desc.inputElements.push_back({ "NORMAL", GraphicsShader::InputElement::ElementType::Float, 3, false });
			desc.inputElements.push_back({ "BINORMAL", GraphicsShader::InputElement::ElementType::Float, 3, false });
			desc.inputElements.push_back({ "BONEINDICES", GraphicsShader::InputElement::ElementType::Int, 4, false });
			desc.inputElements.push_back({ "BONEWEIGHTS", GraphicsShader::InputElement::ElementType::Float, 4, false });
		}));
		RegisterKey("std3d_geometry_skeletal", handle);

		handle = g_registeredAssets.size();
		RegisterAsset(handle, CreateRef<GraphicsShaderAsset>([](GraphicsShaderAsset::Descriptor& desc) {
			desc.shaderPath = "Resources/Shaders/lighting3d_point.fx";
			desc.shaderCompileFlags = ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel;
			desc.inputElements.push_back({ "POSITION", GraphicsShader::InputElement::ElementType::Float, 3, false });
		}));
		RegisterKey("lighting3d_point", handle);

		handle = g_registeredAssets.size();
		RegisterAsset(handle, CreateRef<GraphicsShaderAsset>([](GraphicsShaderAsset::Descriptor& desc) {
			desc.shaderPath = "Resources/Shaders/lighting3d_spot.fx";
			desc.shaderCompileFlags = ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel;
			desc.inputElements.push_back({ "POSITION", GraphicsShader::InputElement::ElementType::Float, 3, false });
			desc.inputElements.push_back({ "TEXCOORD", GraphicsShader::InputElement::ElementType::Float, 2, false });
			desc.inputElements.push_back({ "TANGENT", GraphicsShader::InputElement::ElementType::Float, 3, false });
			desc.inputElements.push_back({ "NORMAL", GraphicsShader::InputElement::ElementType::Float, 3, false });
			desc.inputElements.push_back({ "BINORMAL", GraphicsShader::InputElement::ElementType::Float, 3, false });
		}));
		RegisterKey("lighting3d_spot", handle);
	}

	void AssetManager::RegisterDefaultMaterials() {
		AssetHandle handle = g_registeredAssets.size();
		RegisterAsset(handle, CreateRef<MaterialAsset>([](MaterialAsset::Descriptor& desc) {
			desc.shaderHandle = AssetManager::GetHandleByKey("std3d_geometry_static");
			desc.renderMode = RenderMode::Opaque;
			desc.cullMode = CullMode::Back;
			desc.depthTest = DepthTest::LessEqual;
			desc.depthWrite = true;
		}));
		RegisterKey("default_material_std3d_geometry", handle);
	}

	void AssetManager::RegisterDefaultStaticMeshs() {
		AssetHandle handle = g_registeredAssets.size();
		RegisterAsset(handle, CreateRef<StaticMeshAsset>([](StaticMeshAsset::Descriptor& desc) {
			GenerateCone([&desc](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) { desc.vertices.emplace_back(Vertex3D{ pos, uv, tangent, normal, binormal }); }, desc.indices, 50, 0.5, 1);
			desc.materials.push_back(AssetManager::GetHandleByKey("default_material_std3d_geometry"));
			desc.segments.emplace_back(MeshSegment{ PrimitiveTopology::TriangleList, 0, (uint32_t)desc.vertices.size(), 0, (uint32_t)desc.indices.size() });
		}));
		RegisterKey("default_static_cone_mesh", handle);

		handle = g_registeredAssets.size();
		RegisterAsset(handle, CreateRef<StaticMeshAsset>([](StaticMeshAsset::Descriptor& desc) {
			GenerateCube([&desc](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) { desc.vertices.emplace_back(Vertex3D{ pos, uv, tangent, normal, binormal }); }, desc.indices);
			desc.materials.push_back(AssetManager::GetHandleByKey("default_material_std3d_geometry"));
			desc.segments.emplace_back(MeshSegment{ PrimitiveTopology::TriangleList, 0, (uint32_t)desc.vertices.size(), 0, (uint32_t)desc.indices.size() });
		}));
		RegisterKey("default_static_cube_mesh", handle);

		handle = g_registeredAssets.size();
		RegisterAsset(handle, CreateRef<StaticMeshAsset>([](StaticMeshAsset::Descriptor& desc) {
			GenerateSphere([&desc](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) { desc.vertices.emplace_back(Vertex3D{ pos, uv, tangent, normal, binormal }); }, desc.indices, 50, 50, 0.5);
			desc.materials.push_back(AssetManager::GetHandleByKey("default_material_std3d_geometry"));
			desc.segments.emplace_back(MeshSegment{ PrimitiveTopology::TriangleList, 0, (uint32_t)desc.vertices.size(), 0, (uint32_t)desc.indices.size() });
		}));
		RegisterKey("default_static_sphere_mesh", handle);

		handle = g_registeredAssets.size();
		RegisterAsset(handle, CreateRef<StaticMeshAsset>([](StaticMeshAsset::Descriptor& desc) {
			GenerateQuad([&desc](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) { desc.vertices.emplace_back(Vertex3D{ pos, uv, tangent, normal, binormal }); }, desc.indices);
			// NOTE: no material for quad mesh
			desc.segments.emplace_back(MeshSegment{ PrimitiveTopology::TriangleList, 0, (uint32_t)desc.vertices.size(), 0, (uint32_t)desc.indices.size() });
		}));
		RegisterKey("default_static_quad_mesh", handle);
	}

	void AssetManager::Cleanup() {
		g_registeredAssets.clear();
	}

	void AssetManager::RegisterKey(const std::string_view& key, const AssetHandle& handle) {
		if (g_assetKeyMap.find(key.data()) != g_assetKeyMap.end()) {
			return;
		}
		g_assetKeyMap[key.data()] = handle;
	}

	void AssetManager::UnregisterKey(const std::string_view& key, const AssetHandle& handle) {
		auto it = g_assetKeyMap.find(key.data());
		if (it == g_assetKeyMap.end()) {
			return;
		}
		g_assetKeyMap.erase(it);
	}

	AssetHandle AssetManager::GetHandleByKey(const std::string_view& key) {
		auto it = g_assetKeyMap.find(key.data());
		if (it == g_assetKeyMap.end()) {
			return AssetHandle();
		}
		return it->second;
	}

	void AssetManager::RegisterAsset(const AssetHandle& handle, const Ref<Asset>& asset) {
		if (IsAssetRegistered(handle)) {
			return;
		}
		
		// NOTE: Only register the asset, do not load it yet
		g_registeredAssets[handle] = asset;
	}

	void AssetManager::UnregisterAsset(const AssetHandle& handle) {
		auto it = g_registeredAssets.find(handle);
		if (it == g_registeredAssets.end()) {
			return;
		}

		g_registeredAssets.erase(it);
	}

	void AssetManager::LoadAsset(const AssetHandle& handle) {
		auto it = g_registeredAssets.find(handle);
		if (it == g_registeredAssets.end()) {
			return;
		}

		if (it->second->IsLoaded()) {
			return;
		}

		it->second->Load();
	}

	void AssetManager::UnloadAsset(const AssetHandle& handle) {
		auto it = g_registeredAssets.find(handle);
		if (it == g_registeredAssets.end()) {
			return;
		}

		if (!it->second->IsLoaded()) {
			return;
		}

		it->second->Unload();
	}

	void AssetManager::ReloadAsset(const AssetHandle& handle) {
		auto it = g_registeredAssets.find(handle);
		if (it == g_registeredAssets.end()) {
			return;
		}

		if (it->second->IsLoaded()) {
			it->second->Unload();
		}

		it->second->Load();
	}

	Ref<Asset> AssetManager::GetAsset(const AssetHandle& handle) {
		auto it = g_registeredAssets.find(handle);
		if (it == g_registeredAssets.end()) {
			return nullptr;
		}

		if (!it->second->IsLoaded()) {
			it->second->Load();
		}

		return it->second;
	}

	Ref<Asset> AssetManager::GetAsset(const std::string_view& key) {
		auto it = g_assetKeyMap.find(key.data());
		if (it == g_assetKeyMap.end()) {
			return nullptr;
		}

		return GetAsset(it->second);
	}

	bool AssetManager::IsAssetRegistered(const AssetHandle& handle) {
		return g_registeredAssets.find(handle) != g_registeredAssets.end();
	}

	void AssetManager::EachAssets(const std::function<void(const AssetHandle&, const Ref<Asset>&)>& func) {
		for (const auto& [handle, asset] : g_registeredAssets) {
			func(handle, asset);
		}
	}

	AssetHandle AssetManager::GenerateNewAssetHandle() {
		AssetHandle handle;
		handle.Generate();

		while (IsAssetRegistered(handle)) {
			handle.Generate();
		}

		return handle;
	}
}
