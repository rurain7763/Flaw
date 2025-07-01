#pragma once

#include <Flaw.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <inttypes.h>
#include <filesystem>

namespace flaw {
	class EditorHelper {
	public:
		static void DrawTitle(const std::string& title, const vec3& color = { 0.2f, 0.7f, 0.3f }) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.x, color.y, color.z, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color.x, color.y, color.z, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(color.x, color.y, color.z, 1.0f));
			ImGui::Button(title.c_str());
			ImGui::PopStyleColor(3);
		}

		template<typename T>
		static bool DrawNumericInput(const char* label, T& value, T resetValue = 0, float step = 1.0f, float columnWidth = 100.f) {
			bool dirty = false;
			ImGui::PushID(label);
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, columnWidth);

			ImGui::Text(label);

			ImGui::NextColumn();

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

			if constexpr (std::is_same_v<T, float>) {
				if (ImGui::DragFloat("##Input", &value, step)) {
					dirty |= true;
				}
			}
			else if constexpr (std::is_same_v<T, int32_t>) {
				if (ImGui::DragInt("##Input", &value, step)) {
					dirty |= true;
				}
			}
			else if constexpr (std::is_same_v<T, uint32_t>) {
				if (ImGui::DragInt("##Input", &value, step)) {
					dirty |= true;
				}
			}

			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.05f, 0.1f, 1.0f));
			if (ImGui::Button("Reset")) {
				value = resetValue;
				dirty |= true;
			}
			ImGui::PopStyleColor(3);
			
			ImGui::PopStyleVar();
			ImGui::Columns(1);
			ImGui::PopID();

			return dirty;
		}

		static bool DrawCheckbox(const char* label, bool& value, float columnWidth = 100.f) {
			bool dirty = false;
			ImGui::PushID(label);
			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, columnWidth);
			ImGui::Text(label);
			ImGui::NextColumn();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			if (ImGui::Checkbox("##Checkbox", &value)) {
				dirty |= true;
			}
			ImGui::PopStyleVar();
			ImGui::Columns(1);
			ImGui::PopID();
			return dirty;
		}

		static bool DrawInputText(const char* label, std::string& value) {
			bool dirty = false;
			ImGui::PushID(label);
			ImGui::Text(label);
			ImGui::SameLine();

			char buffer[256];
			strncpy(buffer, value.c_str(), sizeof(buffer));
			if (ImGui::InputText("##InputText", buffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
				value = buffer;
				dirty |= true;
			}

			ImGui::PopID();

			return dirty;
		}

		static bool DrawCombo(const char* label, int32_t& value, const std::vector<std::string>& items) {
			bool dirty = false;

			ImGui::PushID(label);

			ImGui::Text("%s", label);

			ImGui::SameLine();

			std::string current = (value >= 0 && value < items.size()) ? items[value] : "None";

			ImGui::SetNextItemWidth(150.f);
			if (ImGui::BeginCombo("##Combo", current.c_str())) {
				for (int32_t i = 0; i < items.size(); i++) {
					bool isSelected = items[i] == current;
					if (ImGui::Selectable(items[i].c_str(), isSelected)) {
						dirty |= value != i;
						value = i;
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::PopID();

			return dirty;
		}

		static bool DrawVec3(const char* label, vec3& vec, float resetValue = 0, float columnWidth = 100.f) {
			bool changed = false;

			ImGuiIO& io = ImGui::GetIO();
			ImFont* font = io.Fonts->Fonts[0];

			ImGui::PushID(label);

			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, columnWidth);
			ImGui::Text(label);
			ImGui::NextColumn();

			ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.05f, 0.1f, 1.0f));
			ImGui::PushFont(font);
			if (ImGui::Button("X", buttonSize)) {
				vec.x = resetValue;
				changed = true;
			}
			ImGui::PopFont();
			ImGui::PopStyleColor(3);

			ImGui::SameLine();

			if (ImGui::DragFloat("##X", &vec.x, 0.1f)) {
				vec.x = vec.x;
				changed = true;
			}

			ImGui::PopItemWidth();
			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.2f, 1.0f));
			ImGui::PushFont(font);
			if (ImGui::Button("Y", buttonSize)) {
				vec.y = resetValue;
				changed = true;
			}
			ImGui::PopFont();
			ImGui::PopStyleColor(3);

			ImGui::SameLine();

			if (ImGui::DragFloat("##Y", &vec.y, 0.1f)) {
				vec.y = vec.y;
				changed = true;
			}

			ImGui::PopItemWidth();
			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.3f, 0.7f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.2f, 0.6f, 1.0f));
			ImGui::PushFont(font);
			if (ImGui::Button("Z", buttonSize)) {
				vec.z = resetValue;
				changed = true;
			}
			ImGui::PopFont();
			ImGui::PopStyleColor(3);

			ImGui::SameLine();

			if (ImGui::DragFloat("##Z", &vec.z, 0.1f)) {
				vec.z = vec.z;
				changed = true;
			}

			ImGui::PopItemWidth();

			ImGui::PopStyleVar();

			ImGui::Columns(1);

			ImGui::PopID();

			return changed;
		}

		static void ShowScrollingText(const char* label, const char* text, ImVec2 boxSize, float scrollSpeed = 30.0f) {
			ImGui::BeginGroup();

			ImVec2 pos = ImGui::GetCursorScreenPos();

			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImGuiID id = window->GetID(label);
			ImVec2 textSize = ImGui::CalcTextSize(text);

			float itemHeight = textSize.y;
			ImVec2 textRegionSize = ImVec2(boxSize.x, itemHeight);

			// Invisible button to detect hover
			ImGui::InvisibleButton(label, textRegionSize);
			bool isHovered = ImGui::IsItemHovered();

			float availableWidth = textRegionSize.x;
			float overflow = textSize.x - availableWidth;

			float scrollOffset = 0.0f;

			if (overflow > 0.0f) {
				static std::unordered_map<ImGuiID, float> scrollOffsets;
				static std::unordered_map<ImGuiID, double> lastResetTime;

				double currentTime = ImGui::GetTime();

				if (!isHovered) {
					// Reset animation when not hovered
					scrollOffsets[id] = 0.0f;
					lastResetTime[id] = currentTime;
				}
				else {
					const float pauseDuration = 3.0f; // Pause duration at the end of the scroll

					float totalScrollTime = overflow / scrollSpeed;
					float elapsed = (float)(currentTime - lastResetTime[id]);

					float cycleTime = totalScrollTime + pauseDuration;
					float t = fmodf(elapsed, cycleTime);

					if (t < totalScrollTime) {
						scrollOffset = t * scrollSpeed;
					}
					else {
						scrollOffset = overflow; // pause at end
					}

					scrollOffsets[id] = scrollOffset;
				}

				scrollOffset = scrollOffsets[id];
			}

			// Clip and draw
			ImGui::SetCursorScreenPos(pos);
			ImGui::PushClipRect(pos, ImVec2(pos.x + textRegionSize.x, pos.y + textRegionSize.y), true);
			ImGui::SetCursorScreenPos(ImVec2(pos.x - scrollOffset, pos.y));
			ImGui::TextUnformatted(text);
			ImGui::PopClipRect();

			ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + textRegionSize.y));
			ImGui::EndGroup();
		}

		static bool DrawAssetPayloadTarget(const char* label, AssetHandle current, const std::function<void(const char*)>& onFileDropped) {
			bool dirty = false;
			
			ImGui::Text("%s", label);

			ImGui::SameLine();

			if (current.IsValid()) {
				if (AssetManager::IsAssetRegistered(current)) {
					ImGui::Text("%" PRIu64, current);
				}
				else {
					ImGui::Text("Invalid Asset");
				}

			}
			else {
				ImGui::Text("None");
			}
			
			if (ImGui::BeginDragDropTarget()) {
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH");
				if (payload) {
					const char* filePath = (const char*)payload->Data;
					onFileDropped(filePath);
					dirty = true;
				}
				ImGui::EndDragDropTarget();
			}

			return dirty;
		}

		static void DrawEntityPayloadTarget(const char* label, Ref<Scene> scene, UUID uuid, const std::function<bool(Entity)>& checkComponent, const std::function<void(Entity)>& onEntityDropped) {
			ImGui::Text("%s", label);

			ImGui::SameLine();

			Entity current = scene->FindEntityByUUID(uuid);

			if (current) {
				if (checkComponent(current)) {
					ImGui::Text("%" PRIu64, uuid);
				}
				else {
					ImGui::Text("Invalid");
				}
			}
			else {
				ImGui::Text("None");
			}

			if (ImGui::BeginDragDropTarget()) {
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID");
				if (payload) {
					entt::entity id = *(entt::entity*)payload->Data;
					Entity entity(id, scene.get());
					if (checkComponent(entity)) {
						onEntityDropped(entity);
					}
				}
				ImGui::EndDragDropTarget();
			}
		}

		static bool DrawInputFilePath(const char* label, std::filesystem::path& filePath, const char* filter) {
			bool dirty = false;

			ImGui::PushID(label); // 고유 ID 스코프

			ImGui::Text("%s", label);
			ImGui::SameLine();

			// 가변 버퍼를 사용한 InputText
			char buffer[512];
			strncpy(buffer, filePath.generic_u8string().c_str(), sizeof(buffer));
			buffer[sizeof(buffer) - 1] = '\0';

			if (ImGui::InputText("##FilePath", buffer, sizeof(buffer))) {
				filePath = buffer;
				dirty = true;
			}

			if (ImGui::IsItemHovered()) {
				if (std::filesystem::exists(filePath)) {
					ImGui::SetTooltip(filePath.generic_u8string().c_str());
				}
				else {
					ImGui::SetTooltip("File does not exist");
				}
			}

			ImGui::SameLine();

			// 파일 선택 버튼
			if (ImGui::Button("...")) {
				std::filesystem::path selectedFile = FileDialogs::OpenFile(Platform::GetPlatformContext(), filter);
				if (!selectedFile.empty()) {
					filePath = selectedFile;
					dirty = true;
				}
			}

			ImGui::SameLine();

			// 파일 경로 초기화 버튼
			if (ImGui::Button("x")) {
				filePath.clear();
				dirty = true;
			}

			ImGui::PopID();

			return dirty;
		}

		template<typename T>
		static bool DrawList(const char* label, std::vector<T>& items, const std::function<bool(T&)>& itemDrawFunc) {
			bool dirty = false;

			ImGui::PushID(label);

			if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetColorU32(ImGuiCol_FrameBg));

				ImGui::BeginChild("ItemListArea", ImVec2(0, 180), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
				if (items.empty()) {
					ImGui::TextDisabled("No items.");
				}
				else {
					for (size_t i = 0; i < items.size(); ) {
						ImGui::PushID(static_cast<int>(i));

						ImGui::Columns(2, nullptr, false);
						dirty |= itemDrawFunc(items[i]);

						ImGui::NextColumn();
						if (ImGui::Button("Remove")) {
							items.erase(items.begin() + i);
							dirty = true;
						}
						else {
							++i;
						}

						ImGui::Columns(1);
						ImGui::Separator();
						ImGui::PopID();
					}
				}
				ImGui::EndChild();

				ImGui::PopStyleColor();
				ImGui::Spacing();

				ImGui::Separator();
				ImGui::BeginChild("ListControls", ImVec2(0, 40), false);
				ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - 60);

				if (ImGui::Button("Add item", ImVec2(60, 0))) {
					items.emplace_back();
					dirty = true;
				}

				ImGui::EndChild();
				ImGui::PopStyleVar();
			}

			ImGui::PopID();

			return dirty;
		}

		template<typename T>
		static bool DrawList(const char* label, std::vector<T>& items, const std::function<bool(T&)>& itemDrawFunc, const std::function<T()>& createNewItemFunc) {
			bool dirty = false;

			ImGui::PushID(label);

			if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetColorU32(ImGuiCol_FrameBg));

				ImGui::BeginChild("ItemListArea", ImVec2(0, 180), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
				if (items.empty()) {
					ImGui::TextDisabled("No items.");
				}
				else {
					for (size_t i = 0; i < items.size(); ) {
						ImGui::PushID(static_cast<int>(i));

						ImGui::Columns(2, nullptr, false);
						dirty |= itemDrawFunc(items[i]);

						ImGui::NextColumn();
						if (ImGui::Button("Remove")) {
							items.erase(items.begin() + i);
							dirty = true;
						}
						else {
							++i;
						}

						ImGui::Columns(1);
						ImGui::Separator();
						ImGui::PopID();
					}
				}
				ImGui::EndChild();

				ImGui::PopStyleColor();
				ImGui::Spacing();

				ImGui::Separator();
				ImGui::BeginChild("ListControls", ImVec2(0, 40), false);
				ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - 60);

				if (ImGui::Button("Add item", ImVec2(60, 0))) {
					items.emplace_back(createNewItemFunc());
					dirty = true;
				}

				ImGui::EndChild();
				ImGui::PopStyleVar();
			}

			ImGui::PopID();

			return dirty;
		}
	};
}