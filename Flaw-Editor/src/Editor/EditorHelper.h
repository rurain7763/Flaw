#pragma once

#include <Flaw.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <inttypes.h>

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

		static bool DrawImputText(const char* label, std::string& value) {
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

			if (ImGui::BeginCombo(label, items[value].c_str())) {
				for (int32_t i = 0; i < items.size(); i++) {
					bool isSelected = items[i] == items[value];
					if (ImGui::Selectable(items[i].c_str(), isSelected)) {
						value = i;
						dirty |= true;
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

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
	};
}