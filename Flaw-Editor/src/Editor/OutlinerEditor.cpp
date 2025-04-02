#include "OutlinerEditor.h"
#include "EditorEvents.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <filesystem>

namespace flaw {
	OutlinerEditor::OutlinerEditor(Application& app)
		: _app(app)
		, _eventDispatcher(app.GetEventDispatcher())
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

		ImGui::Begin("Outliner");

		auto& registry = _scene->GetRegistry();

		for (auto&& [entity, enttComp] : registry.view<EntityComponent>().each()) {
			DrawEntityNode(Entity(entity, _scene.get()));
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

			ImGui::EndPopup();
		}

		ImGui::EndChild();

		ImGui::End();

		// handle command
		if (Input::GetKey(KeyCode::LCtrl)) {
			if (Input::GetKeyDown(KeyCode::D)) {
				if (_selectedEntt) {
					Entity newEntity = _scene->CloneEntity(_selectedEntt);
					_selectedEntt = newEntity;
					_eventDispatcher.Dispatch<OnSelectEntityEvent>(_selectedEntt);
				}
			}
		}
	}

	void OutlinerEditor::DrawEntityNode(const Entity& entity) {
		auto& goc = entity.GetComponent<EntityComponent>();

		const bool isSelected = _selectedEntt == entity;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

		if (isSelected) {
			flags |= ImGuiTreeNodeFlags_Selected;  // 선택된 엔티티 강조
		}

		char label[256];
		sprintf_s(label, "%s##%d", goc.name.c_str(), (uint32_t)entity);

		bool open = ImGui::TreeNodeEx(label, flags);

		if (ImGui::IsItemClicked()) {
			_selectedEntt = entity;
			_eventDispatcher.Dispatch<OnSelectEntityEvent>(_selectedEntt);
		}

		if (open) {
			ImGui::TreePop();
		}
	}
}