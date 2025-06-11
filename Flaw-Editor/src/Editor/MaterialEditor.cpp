#include "MaterialEditor.h"
#include "Editor/EditorHelper.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <filesystem>

namespace flaw {
	MaterialEditor::MaterialEditor(Application& app, const std::string& editorName, const char* assetFile)
		: _app(app)
		, _editorName(editorName)
	{
		_dirty = false;

		AssetMetadata metadata;
		if (!AssetDatabase::GetAssetMetadata(assetFile, metadata)) {
			Log::Error("MaterialEditor: Failed to get asset metadata for %s", assetFile);
			return;
		}

		_targetMaterialAsset = AssetManager::GetAsset<MaterialAsset>(metadata.handle);
		if (!_targetMaterialAsset) {
			return;
		}

		MaterialAsset::Descriptor desc;
		_targetMaterialAsset->GetDescriptor(desc);

		Ref<Material> material = _targetMaterialAsset->GetMaterial();

		_editedMaterialSettings.destPath = assetFile;
		_editedMaterialSettings.shaderHandle = desc.shaderHandle;
		_editedMaterialSettings.renderMode = desc.renderMode;
		_editedMaterialSettings.cullMode = desc.cullMode;
		_editedMaterialSettings.depthTest = desc.depthTest;
		_editedMaterialSettings.depthWrite = desc.depthWrite;
		_editedMaterialSettings.albedoTexture = desc.albedoTexture;
		_editedMaterialSettings.normalTexture = desc.normalTexture;
		_editedMaterialSettings.emissiveTexture = desc.emissiveTexture;
		_editedMaterialSettings.metallicTexture = desc.metallicTexture;
		_editedMaterialSettings.roughnessTexture = desc.roughnessTexture;
		_editedMaterialSettings.ambientOcclusionTexture = desc.ambientOcclusionTexture;
	}

	void MaterialEditor::OnRender() {
		if (!_targetMaterialAsset) {
			return;
		}

		Ref<Material> material = _targetMaterialAsset->GetMaterial();
		
		ImGui::Begin(_editorName.c_str(), &_isOpen);

		std::vector<std::string> renderModeStrs = {
			"Opaque",
			"Masked",
			"Transparent"
		};

		int32_t selectedRenderMode = (int32_t)_editedMaterialSettings.renderMode;
		if (EditorHelper::DrawCombo("Render Mode", selectedRenderMode, renderModeStrs)) {
			_editedMaterialSettings.renderMode = (RenderMode)selectedRenderMode;
			_dirty = true;
		}

		std::vector<std::string> cullModeStrs = {
			"None",
			"Front",
			"Back",
		};

		int32_t selectedCullMode = (int32_t)_editedMaterialSettings.cullMode;
		if (EditorHelper::DrawCombo("Cull Mode", selectedCullMode, cullModeStrs)) {
			_editedMaterialSettings.cullMode = (CullMode)selectedCullMode;
			_dirty = true;
		}

		std::vector<std::string> depthTestStrs = {
			"Less",
			"LessEqual",
			"Greater",
			"GreaterEqual",
			"Equal",
			"NotEqual",
			"Always",
			"Never",
			"Disabled",
		};

		int32_t selectedDepthTest = (int32_t)_editedMaterialSettings.depthTest;
		if (EditorHelper::DrawCombo("Depth Test", selectedDepthTest, depthTestStrs)) {
			_editedMaterialSettings.depthTest = (DepthTest)selectedDepthTest;
			_dirty = true;
		}

		if (ImGui::Checkbox("Depth Write", &_editedMaterialSettings.depthWrite)) {
			_dirty = true;
		}

		ImGui::Separator();

		EditorHelper::DrawAssetPayloadTarget("Shader", _editedMaterialSettings.shaderHandle, [&](const char* filePath) {
			AssetMetadata metadata;
			if (AssetDatabase::GetAssetMetadata(filePath, metadata) && metadata.type == AssetType::GraphicsShader) {
				if (_editedMaterialSettings.shaderHandle != metadata.handle) {
					_editedMaterialSettings.shaderHandle = metadata.handle;
					_dirty = true;
				}
			}
		});

		std::string shaderInput;
		if (EditorHelper::DrawInputText("Find shader by key", shaderInput)) {
			AssetHandle handle = AssetManager::GetHandleByKey(shaderInput);
			if (handle.IsValid() && handle != _editedMaterialSettings.shaderHandle) {
				_editedMaterialSettings.shaderHandle = handle;
				_dirty = true;
			}
		}

		ImGui::Separator();

		EditorHelper::DrawAssetPayloadTarget("Albedo Texture", _editedMaterialSettings.albedoTexture, [&](const char* filePath) {
			AssetMetadata metadata;
			if (AssetDatabase::GetAssetMetadata(filePath, metadata) && metadata.type == AssetType::Texture2D) {
				if (_editedMaterialSettings.albedoTexture != metadata.handle) {
					_editedMaterialSettings.albedoTexture = metadata.handle;
					_dirty = true;
				}
			}
		});

		EditorHelper::DrawAssetPayloadTarget("Normal Texture", _editedMaterialSettings.normalTexture, [&](const char* filePath) {
			AssetMetadata metadata;
			if (AssetDatabase::GetAssetMetadata(filePath, metadata) && metadata.type == AssetType::Texture2D) {
				if (_editedMaterialSettings.normalTexture != metadata.handle) {
					_editedMaterialSettings.normalTexture = metadata.handle;
					_dirty = true;
				}
			}
		});

		EditorHelper::DrawAssetPayloadTarget("Emissive Texture", _editedMaterialSettings.emissiveTexture, [&](const char* filePath) {
			AssetMetadata metadata;
			if (AssetDatabase::GetAssetMetadata(filePath, metadata) && metadata.type == AssetType::Texture2D) {
				if (_editedMaterialSettings.emissiveTexture != metadata.handle) {
					_editedMaterialSettings.emissiveTexture = metadata.handle;
					_dirty = true;
				}
			}
		});

		EditorHelper::DrawAssetPayloadTarget("Metallic Texture", _editedMaterialSettings.metallicTexture, [&](const char* filePath) {
			AssetMetadata metadata;
			if (AssetDatabase::GetAssetMetadata(filePath, metadata) && metadata.type == AssetType::Texture2D) {
				if (_editedMaterialSettings.metallicTexture != metadata.handle) {
					_editedMaterialSettings.metallicTexture = metadata.handle;
					_dirty = true;
				}
			}
		});

		EditorHelper::DrawAssetPayloadTarget("Roughness Texture", _editedMaterialSettings.roughnessTexture, [&](const char* filePath) {
			AssetMetadata metadata;
			if (AssetDatabase::GetAssetMetadata(filePath, metadata) && metadata.type == AssetType::Texture2D) {
				if (_editedMaterialSettings.roughnessTexture != metadata.handle) {
					_editedMaterialSettings.roughnessTexture = metadata.handle;
					_dirty = true;
				}
			}
		});

		EditorHelper::DrawAssetPayloadTarget("Ambient Occlusion Texture", _editedMaterialSettings.ambientOcclusionTexture, [&](const char* filePath) {
			AssetMetadata metadata;
			if (AssetDatabase::GetAssetMetadata(filePath, metadata) && metadata.type == AssetType::Texture2D) {
				if (_editedMaterialSettings.ambientOcclusionTexture != metadata.handle) {
					_editedMaterialSettings.ambientOcclusionTexture = metadata.handle;
					_dirty = true;
				}
			}
		});

		if (_dirty && ImGui::Button("Save")) {
			AssetDatabase::RecreateAsset(_editedMaterialSettings.destPath.c_str(), &_editedMaterialSettings);
			_isOpen = false;
			_dirty = false;
		}

		ImGui::End();
	}
}