#include "OutlinerEditor.h"
#include "EditorEvents.h"
#include "AssetDatabase.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <filesystem>

namespace flaw {
	OutlinerEditor::OutlinerEditor(Application& app, const std::string& name)
		: _app(app)
		, _eventDispatcher(app.GetEventDispatcher())
		, _name(name)
	{
		_eventDispatcher.Register<OnSelectEntityEvent>([this](const OnSelectEntityEvent& evn) { _selectedEntt = evn.entity; }, PID(this));
	}

	OutlinerEditor::~OutlinerEditor() {
		_eventDispatcher.UnregisterAll(PID(this));
	}

	void OutlinerEditor::SetScene(const Ref<Scene>& scene) {
		_scene = scene;
		_selectedEntt = Entity();
	}

	void OutlinerEditor::OnRender() {
		if (!_scene) {
			return;
		}

		ImGui::Begin(_name.c_str(), &_isOpen);

		static char searchBuffer[128] = { 0 };
		ImGui::InputTextWithHint("##search", "Search...", searchBuffer, sizeof(searchBuffer));
		ImGui::Separator();

		ImGui::BeginChild("##OutlinerChild");

		auto& registry = _scene->GetRegistry();

		for (auto&& [entity, enttComp] : registry.view<EntityComponent>().each()) {
			Entity entt(entity, _scene.get());
			
			if (searchBuffer[0] == '\0') {
				if (!entt.HasParent()) {
					DrawEntityNode(entt, true);
				}
			}
			else if (enttComp.name.find(searchBuffer) != std::string::npos) {
				DrawEntityNode(entt, false);
			}
		}

		// delete selected object
		if (ImGui::IsWindowFocused() && (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) && _selectedEntt) {
			_scene->DestroyEntity(_selectedEntt);
			_selectedEntt = Entity();
			_eventDispatcher.Dispatch<OnSelectEntityEvent>(_selectedEntt);
		}

		// empty space button (handle right click, disconnect parent, etc...)
		ImGui::BeginChild("##EmptySpace", ImGui::GetContentRegionAvail());

		// left click on empty space
		if (ImGui::InvisibleButton("##InvisibleButton", ImGui::GetContentRegionAvail())) {
			_selectedEntt = Entity();
			_eventDispatcher.Dispatch<OnSelectEntityEvent>(_selectedEntt);
		}

		// right click
		if (ImGui::IsMouseClicked(1) && ImGui::IsWindowHovered()) {
			ImGui::OpenPopup("OutlinerContextMenu");
		}

		if (ImGui::BeginPopupContextItem("OutlinerContextMenu")) {
			if (ImGui::MenuItem("Create Empty")) {
				Entity newEntity = _scene->CreateEntity();
				newEntity.GetComponent<EntityComponent>().name = "New Entity";

				_selectedEntt = newEntity;
				_eventDispatcher.Dispatch<OnSelectEntityEvent>(_selectedEntt);
			}

			if (ImGui::MenuItem("Create cube")) {
				Entity newEntity = _scene->CreateEntity();
				newEntity.GetComponent<EntityComponent>().name = "New Cube";
				AssetHandle cubeMeshAsseHandle = AssetManager::GetHandleByKey("default_static_cube_mesh");
				auto& meshAsset = AssetManager::GetAsset<StaticMeshAsset>(cubeMeshAsseHandle);
				if (meshAsset) {
					auto& meshComp = newEntity.AddComponent<StaticMeshComponent>();
					meshComp.mesh = cubeMeshAsseHandle;
					meshComp.materials.clear();
					for (const auto& materialHandle : meshAsset->GetMaterialHandles()) {
						meshComp.materials.push_back(materialHandle);
					}
				}
			}

			if (ImGui::MenuItem("Create sphere")) {
				Entity newEntity = _scene->CreateEntity();
				newEntity.GetComponent<EntityComponent>().name = "New Sphere";
				AssetHandle sphereMeshAsseHandle = AssetManager::GetHandleByKey("default_static_sphere_mesh");
				auto& meshAsset = AssetManager::GetAsset<StaticMeshAsset>(sphereMeshAsseHandle);
				if (meshAsset) {
					auto& meshComp = newEntity.AddComponent<StaticMeshComponent>();
					meshComp.mesh = sphereMeshAsseHandle;
					meshComp.materials.clear();
					for (const auto& materialHandle : meshAsset->GetMaterialHandles()) {
						meshComp.materials.push_back(materialHandle);
					}
				}
			}

			if (ImGui::MenuItem("Create cone")) {
				Entity newEntity = _scene->CreateEntity();
				newEntity.GetComponent<EntityComponent>().name = "New Cone";
				AssetHandle coneMeshAsseHandle = AssetManager::GetHandleByKey("default_static_cone_mesh");

				auto& meshAsset = AssetManager::GetAsset<StaticMeshAsset>(coneMeshAsseHandle);
				if (meshAsset) {
					auto& meshComp = newEntity.AddComponent<StaticMeshComponent>();
					meshComp.mesh = coneMeshAsseHandle;
					meshComp.materials.clear();
					for (const auto& materialHandle : meshAsset->GetMaterialHandles()) {
						meshComp.materials.push_back(materialHandle);
					}
				}
			}

			if (ImGui::MenuItem("Create Canvas")) {
				Entity newEntity = _scene->CreateEntity();
				newEntity.GetComponent<EntityComponent>().name = "New Canvas";
				newEntity.AddComponent<RectLayoutComponent>();
				newEntity.AddComponent<CanvasComponent>();
				newEntity.AddComponent<CanvasScalerComponent>();
			}

			if (ImGui::MenuItem("Create Image")) {
				Entity newEntity = _scene->CreateEntity();
				newEntity.GetComponent<EntityComponent>().name = "New Image";
				newEntity.AddComponent<RectLayoutComponent>();
				newEntity.AddComponent<ImageComponent>();
			}

			ImGui::EndPopup();
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID")) {
				entt::entity id = *(entt::entity*)payload->Data;
				Entity draggedEntity(id, _scene.get());
				draggedEntity.UnsetParent();
			}

			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
				const char* filePath = (const char*)payload->Data;

				AssetMetadata metadata;
				if (AssetDatabase::GetAssetMetadata(filePath, metadata)) {
					if (metadata.type == AssetType::Prefab) {
						auto prefabAsset = AssetManager::GetAsset<PrefabAsset>(metadata.handle);
						prefabAsset->GetPrefab()->CreateEntity(*_scene);
					}
				}
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::EndChild();
		ImGui::EndChild();

		ImGui::End();

		// handle command
		if (Input::GetKey(KeyCode::LCtrl)) {
			if (Input::GetKeyDown(KeyCode::D)) {
				if (_selectedEntt) {
					_selectedEntt = _scene->CloneEntity(_selectedEntt);
					_eventDispatcher.Dispatch<OnSelectEntityEvent>(_selectedEntt);
				}
			}
		}
	}

	void OutlinerEditor::DrawEntityNode(const Entity& entity, bool recursive) {
		auto& goc = entity.GetComponent<EntityComponent>();

		const bool isSelected = _selectedEntt == entity;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

		if (!entity.HasChild() || !recursive) {
			flags |= ImGuiTreeNodeFlags_Leaf;  // 자식이 없는 경우 리프 노드로 표시
		}

		if (isSelected) {
			flags |= ImGuiTreeNodeFlags_Selected;  // 선택된 엔티티 강조
		}

		char label[256];
		sprintf_s(label, "%s##%d", goc.name.c_str(), (entt::entity)entity);

		bool open = ImGui::TreeNodeEx(label, flags);

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) {
			_selectedEntt = entity;
			_eventDispatcher.Dispatch<OnSelectEntityEvent>(_selectedEntt);
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			entt::entity id = entity;
			ImGui::SetDragDropPayload("ENTITY_ID", &id, sizeof(entt::entity));
			ImGui::Text("%s", goc.name.c_str());
			ImGui::EndDragDropSource();
		}

		Entity needToSetParent;
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID")) {
				entt::entity id = *(entt::entity*)payload->Data;
				needToSetParent = Entity(id, _scene.get());
			}
			ImGui::EndDragDropTarget();
		}

		if (open) {
			if (recursive) {
				entity.EachChildren([this, recursive](const Entity& child) {
					DrawEntityNode(child, recursive);
				});
			}

			ImGui::TreePop();
		}

		if (needToSetParent) {
			needToSetParent.SetParent(entity);
		}
	}
}