#pragma once

#include <Flaw.h>
#include "Editor.h"
#include "EditorHelper.h"

namespace flaw {
	class DetailsEditor : public Editor {
	public:
		DetailsEditor(Application& app, const std::string& editorName);
		~DetailsEditor();

		void SetScene(const Ref<Scene>& scene);

		void OnRender() override;

	private:
		template <typename T>
		void DrawComponent(Entity& entity, const std::function<void(T&)>& drawFunc) {
			if (!entity.HasComponent<T>()) {
				return;
			}

			ImGui::Separator();

			const char* name = TypeName<T>().data();

			ImGui::BeginChild(name, ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);

			const ImVec2 size = ImGui::GetContentRegionAvail();

			EditorHelper::DrawTitle(name);

			ImGui::SameLine(size.x - 20);

			if (ImGui::Button("!")) {
				ImGui::OpenPopup("ComponentSetting");
			}

			bool removed = false;
			if (ImGui::BeginPopup("ComponentSetting")) {
				if (ImGui::MenuItem("Remove")) {
					removed = true;
				}
				ImGui::EndPopup();
			}

			std::invoke(drawFunc, entity.GetComponent<T>());

			if (removed) {
				entity.RemoveComponent<T>();
			}

			ImGui::EndChild();
		}

		template <typename T>
		void DrawAddComponentItem(Entity& entity) {
			if (ImGui::MenuItem(TypeName<T>().data()) && !entity.HasComponent<T>()) {
				entity.AddComponent<T>();
			}
		}

	private:
		Application& _app;

		std::string _editorName;

		Ref<Scene> _scene;

		Entity _selectedEntt;
	};
}