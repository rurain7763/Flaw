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
		_editedSkeletonSettings.bones = desc.bones;
		_editedSkeletonSettings.sockets = desc.sockets;
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

		DrawSkeleton(skeleton);
		DrawSockets(skeleton);

		ImGui::Separator();

		if (_dirty && ImGui::Button("Save")) {
			AssetDatabase::RecreateAsset(_editedSkeletonSettings.destPath.c_str(), &_editedSkeletonSettings);
			_isOpen = false;
			_dirty = false;
		}

		ImGui::End();
	}

	void SkeletonEditor::DrawBone(int32_t currentNodeIndex) {
		const auto& currentNode = _editedSkeletonSettings.nodes[currentNodeIndex];

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
		bool hasChildren = !currentNode.childrenIndices.empty();
		bool isBoneNode = std::find_if(_editedSkeletonSettings.bones.begin(), _editedSkeletonSettings.bones.end(), [currentNodeIndex](const SkeletonBoneNode& bone) { return bone.nodeIndex == currentNodeIndex; }) != _editedSkeletonSettings.bones.end();

		if (!isBoneNode) {
			for (int32_t childIndex : currentNode.childrenIndices) {
				DrawBone(childIndex);
			}

			return;
		}

		if (!hasChildren) {
			flags |= ImGuiTreeNodeFlags_Leaf;
		}

		bool open = ImGui::TreeNodeEx(currentNode.name.c_str(), flags);

		if (open) {
			for (int32_t childIndex : currentNode.childrenIndices) {
				DrawBone(childIndex);
			}

			ImGui::TreePop();
		}
	}

	void SkeletonEditor::DrawSkeleton(Ref<Skeleton> skeleton) {
		ImGui::BeginChild("SkeletonTree", ImVec2(0, 300), true);
		DrawBone(0);
		ImGui::EndChild();
	}

	void SkeletonEditor::DrawSockets(Ref<Skeleton> skeleton) {
		std::vector<std::string> boneNames;

		std::transform(_editedSkeletonSettings.bones.begin(), _editedSkeletonSettings.bones.end(), std::back_inserter(boneNames), [this](const SkeletonBoneNode& bone) { 
			return _editedSkeletonSettings.nodes[bone.nodeIndex].name;
		});

		auto itemDrawFunc = [this, &boneNames](SkeletonBoneSocket& socket) -> bool {
			_dirty |= EditorHelper::DrawCombo("Bone", socket.boneIndex, boneNames);
			_dirty |= EditorHelper::DrawInputText("Socket Name", socket.name);

			vec3 position = ExtractPosition(socket.localTransform);
			vec3 rotation = ExtractRotation(socket.localTransform);

			_dirty |= EditorHelper::DrawVec3("Position", position);
			_dirty |= EditorHelper::DrawVec3("Rotation", rotation);

			socket.localTransform = ModelMatrix(position, rotation, vec3(1.0f));

			return _dirty;
		};

		auto createNewItemFunc = [this]() -> SkeletonBoneSocket {
			std::string newSocketName = "New Socket";
			int32_t counter = 1;
			while (std::any_of(_editedSkeletonSettings.sockets.begin(), _editedSkeletonSettings.sockets.end(), [&newSocketName](const SkeletonBoneSocket& socket) { return socket.name == newSocketName; })) {
				newSocketName = "New Socket " + std::to_string(counter++);
			}
			
			SkeletonBoneSocket newSocket;
			newSocket.name = newSocketName;
			newSocket.boneIndex = -1; // Default to -1, can be set later
			newSocket.localTransform = mat4(1.0f); // Default to identity matrix

			_dirty = true;

			return newSocket;
		};

		EditorHelper::DrawList<SkeletonBoneSocket>("Bone Sockets", _editedSkeletonSettings.sockets, itemDrawFunc, createNewItemFunc);
	}
}
