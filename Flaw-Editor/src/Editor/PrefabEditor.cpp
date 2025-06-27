#include "PrefabEditor.h"
#include "Editor/EditorHelper.h"
#include "Log/Log.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace flaw {
	PrefabEditor::PrefabEditor(Application& app, const std::string& editorName, const char* assetFile)
		: _app(app)
		, _editorName(editorName)
		, _outlinerEditor(app, editorName + "_Outliner")
		, _detailsEditor(app, editorName + "_Details")
	{
		AssetMetadata metadata;
		if (!AssetDatabase::GetAssetMetadata(assetFile, metadata)) {
			Log::Error("PrefabEditor: Failed to get asset metadata for %s", assetFile);
			return;
		}

		_targetPrefabAsset = AssetManager::GetAsset<PrefabAsset>(metadata.handle);
		if (!_targetPrefabAsset) {
			return;
		}

		PrefabAsset::Descriptor desc;
		_targetPrefabAsset->GetDescriptor(desc);

		_editedPrefabSettings.destPath = assetFile;
		_editedPrefabSettings.prefabData = desc.prefabData;

		_scene = CreateRef<Scene>(app);
		_rootEntity = _targetPrefabAsset->GetPrefab()->CreateEntity(*_scene);

		_outlinerEditor.SetScene(_scene);
		_detailsEditor.SetScene(_scene);
	}

	void PrefabEditor::OnRender() {
		if (!_targetPrefabAsset) {
			return;
		}

		ImGui::Begin(_editorName.c_str(), &_isOpen, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking);

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Save")) {
					// TODO: Check if the root entity is valid before saving, you must prevent deleting the root entity from the scene,
					// but for now, we just simply check if the root entity exists, if not we log an error.
					if (_rootEntity) {
						_editedPrefabSettings.prefabData = Prefab::ExportData(_rootEntity);
						AssetDatabase::RecreateAsset(_editedPrefabSettings.destPath.c_str(), &_editedPrefabSettings);
					}
					else {
						Log::Error("PrefabEditor: No root entity found to save prefab. Maybe you deleted the root entity.");
					}
					_isOpen = false;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGuiID dockspaceID = ImGui::GetID((_editorName + "_DockSpace").c_str());
		ImGui::DockSpace(dockspaceID, ImVec2(0, 0), ImGuiDockNodeFlags_None);

		ImGui::End();

		_outlinerEditor.OnRender();
		_detailsEditor.OnRender();
	}
}