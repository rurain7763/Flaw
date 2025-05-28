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

		std::string shaderName = _editedMaterialSettings.shaderHandle.IsValid() ? std::to_string(_editedMaterialSettings.shaderHandle) : "Null";
		EditorHelper::DrawAssetPayloadTarget(("Shader: " + shaderName).c_str(), [&](const char* filePath) {
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

		std::string albedoTextureName = _editedMaterialSettings.albedoTexture.IsValid() ? std::to_string(_editedMaterialSettings.albedoTexture) : "Null";
		EditorHelper::DrawAssetPayloadTarget(("Albedo Texture: " + albedoTextureName).c_str(), [&](const char* filePath) {
			AssetMetadata metadata;
			if (AssetDatabase::GetAssetMetadata(filePath, metadata) && metadata.type == AssetType::Texture2D) {
				if (_editedMaterialSettings.albedoTexture != metadata.handle) {
					_editedMaterialSettings.albedoTexture = metadata.handle;
					_dirty = true;
				}
			}
		});

		std::string normalTextureName = _editedMaterialSettings.normalTexture.IsValid() ? std::to_string(_editedMaterialSettings.normalTexture) : "Null";
		EditorHelper::DrawAssetPayloadTarget(("Normal Texture: " + normalTextureName).c_str(), [&](const char* filePath) {
			AssetMetadata metadata;
			if (AssetDatabase::GetAssetMetadata(filePath, metadata) && metadata.type == AssetType::Texture2D) {
				if (_editedMaterialSettings.normalTexture != metadata.handle) {
					_editedMaterialSettings.normalTexture = metadata.handle;
					_dirty = true;
				}
			}
		});

		std::string emissiveTextureName = _editedMaterialSettings.emissiveTexture.IsValid() ? std::to_string(_editedMaterialSettings.emissiveTexture) : "Null";
		EditorHelper::DrawAssetPayloadTarget(("Emissive Texture: " + emissiveTextureName).c_str(), [&](const char* filePath) {
			AssetMetadata metadata;
			if (AssetDatabase::GetAssetMetadata(filePath, metadata) && metadata.type == AssetType::Texture2D) {
				if (_editedMaterialSettings.emissiveTexture != metadata.handle) {
					_editedMaterialSettings.emissiveTexture = metadata.handle;
					_dirty = true;
				}
			}
		});

		std::string metallicTextureName = _editedMaterialSettings.metallicTexture.IsValid() ? std::to_string(_editedMaterialSettings.metallicTexture) : "Null";
		EditorHelper::DrawAssetPayloadTarget(("Metallic Texture: " + metallicTextureName).c_str(), [&](const char* filePath) {
			AssetMetadata metadata;
			if (AssetDatabase::GetAssetMetadata(filePath, metadata) && metadata.type == AssetType::Texture2D) {
				if (_editedMaterialSettings.metallicTexture != metadata.handle) {
					_editedMaterialSettings.metallicTexture = metadata.handle;
					_dirty = true;
				}
			}
		});

		std::string roughnessTextureName = _editedMaterialSettings.roughnessTexture.IsValid() ? std::to_string(_editedMaterialSettings.roughnessTexture) : "Null";
		EditorHelper::DrawAssetPayloadTarget(("Roughness Texture: " + roughnessTextureName).c_str(), [&](const char* filePath) {
			AssetMetadata metadata;
			if (AssetDatabase::GetAssetMetadata(filePath, metadata) && metadata.type == AssetType::Texture2D) {
				if (_editedMaterialSettings.roughnessTexture != metadata.handle) {
					_editedMaterialSettings.roughnessTexture = metadata.handle;
					_dirty = true;
				}
			}
		});

		std::string ambientOcclusionTextureName = _editedMaterialSettings.ambientOcclusionTexture.IsValid() ? std::to_string(_editedMaterialSettings.ambientOcclusionTexture) : "Null";
		EditorHelper::DrawAssetPayloadTarget(("Ambient Occlusion Texture: " + ambientOcclusionTextureName).c_str(), [&](const char* filePath) {
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