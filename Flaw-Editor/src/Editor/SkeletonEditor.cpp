#include "SkeletonEditor.h"
#include "Editor/EditorHelper.h"

namespace flaw {
	SkeletonEditor::SkeletonEditor(Application& app, const std::string& editorName, const char* assetFile) 
		: _app(app)
		, _editorName(editorName)
	{
		_dirty = false;

		AssetMetadata metadata;
		if (!AssetDatabase::GetAssetMetadata(assetFile, metadata)) {
			Log::Error("SkeletonEditor: Failed to get asset metadata for %s", assetFile);
			return;
		}

		_targetSkeletonAsset = AssetManager::GetAsset<SkeletonAsset>(metadata.handle);
		if (!_targetSkeletonAsset) {
			return;
		}

		SkeletonAsset::Descriptor desc;
		_targetSkeletonAsset->GetDescriptor(desc);

		_editedSkeletonSettings.destPath = assetFile;
		_editedSkeletonSettings.globalInvMatrix = desc.globalInvMatrix;
		_editedSkeletonSettings.nodes = desc.nodes;
		_editedSkeletonSettings.boneMap = desc.boneMap;
		_editedSkeletonSettings.animationHandles = desc.animationHandles;
	}

	void SkeletonEditor::OnRender() {
		if (!_targetSkeletonAsset) {
			return;
		}

		Ref<Skeleton> skeleton = _targetSkeletonAsset->GetSkeleton();
		ImGui::Begin(_editorName.c_str(), &_isOpen);

		auto animationItemDrawFunc = [](AssetHandle& handle) {
			return EditorHelper::DrawAssetPayloadTarget("Animation", handle, [&handle](const char* filePath) {
				AssetMetadata metadata;
				if (AssetDatabase::GetAssetMetadata(filePath, metadata) && metadata.type == AssetType::SkeletalAnimation) {
					handle = metadata.handle;
				}
			});
		};

		_dirty |= EditorHelper::DrawList<AssetHandle>("Animations", _editedSkeletonSettings.animationHandles, animationItemDrawFunc);

		ImGui::Separator();

		if (_dirty && ImGui::Button("Save")) {
			AssetDatabase::RecreateAsset(_editedSkeletonSettings.destPath.c_str(), &_editedSkeletonSettings);
			_isOpen = false;
			_dirty = false;
		}

		ImGui::End();
	}
}
