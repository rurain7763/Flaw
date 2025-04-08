#include "pch.h"
#include "DetailsEditor.h"
#include "EditorEvents.h"
#include "AssetDatabase.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <inttypes.h>

namespace flaw {
	// TODO: this code must be some where else
	static void DrawTitle(const std::string& title, const vec3& color = { 0.2f, 0.7f, 0.3f }) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.x, color.y, color.z, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color.x, color.y, color.z, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(color.x, color.y, color.z, 1.0f));
		ImGui::Button(title.c_str());
		ImGui::PopStyleColor(3);
	}

	// TODO: this code must be some where else
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

	// TODO: this code must be some where else
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

	// TODO: this code must be some where else
	template <typename T>
	void DrawComponent(const char* name, Entity& entity, const std::function<void(T&)>& drawFunc) {
		if (!entity.HasComponent<T>()) {
			return;
		}

		ImGui::Separator();

		ImGui::BeginChild(name, ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);

		const ImVec2 size = ImGui::GetContentRegionAvail();

		DrawTitle(name);

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
	static void DrawAddComponentItem(const char* name, Entity& entity) {
		if (ImGui::MenuItem(name) && !entity.HasComponent<T>()) {
			entity.AddComponent<T>();
		}
	}

	DetailsEditor::DetailsEditor(Application& app)
		: _app(app) 
	{
		auto& eventDispatcher = _app.GetEventDispatcher();
		eventDispatcher.Register<OnSelectEntityEvent>([this](const OnSelectEntityEvent& evn) { _selectedEntt = evn.entity; }, PID(this));
	}

	DetailsEditor::~DetailsEditor() {
		auto& eventDispatcher = _app.GetEventDispatcher();
		eventDispatcher.UnregisterAll(PID(this));
	}

	void DetailsEditor::OnRender() {
		ImGui::Begin("Details");

		if (_selectedEntt) {
			auto& entityComp = _selectedEntt.GetComponent<EntityComponent>();

			ImGui::Text("ID: %d", (uint32_t)_selectedEntt);
			ImGui::Text("UUID: %" PRIu64, entityComp.uuid);

			char nameBuffer[256] = { 0 };
			strcpy_s(nameBuffer, entityComp.name.c_str());
			if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
				entityComp.name = nameBuffer;
			}

			DrawComponent<TransformComponent>("Transform", _selectedEntt, [](TransformComponent& transformComp) {
				DrawVec3("Position", transformComp.position);

				vec3 degreeRotation = glm::degrees(transformComp.rotation);
				if (DrawVec3("Rotation", degreeRotation, 0.f, 100.f)) {
					transformComp.rotation = glm::radians(degreeRotation);
				}

				DrawVec3("Scale", transformComp.scale);
			});

			DrawComponent<CameraComponent>("Camera", _selectedEntt, [](CameraComponent& cameraComp) {
				bool perspective = cameraComp.perspective;

				int32_t projectionSelected = perspective ? 1 : 0;
				if (DrawCombo("Projection", projectionSelected, { "Orthographic", "Perspective" })) {
					cameraComp.perspective = projectionSelected == 1;
				}

				if (cameraComp.perspective) {
					float degreeFov = glm::degrees(cameraComp.fov);
					if (ImGui::DragFloat("FOV", &degreeFov, 1.f)) {
						cameraComp.fov = glm::radians(degreeFov);
					}
				}
				else {
					ImGui::DragFloat("Ortho Size", &cameraComp.orthoSize, 0.1f);
				}

				ImGui::DragFloat("Aspect Ratio", &cameraComp.aspectRatio, 0.1f);
				ImGui::DragFloat("Near Clip", &cameraComp.nearClip, 0.1f);
				ImGui::DragFloat("Far Clip", &cameraComp.farClip, 0.1f);
			});

			DrawComponent<SpriteRendererComponent>("Sprite Renderer", _selectedEntt, [this](SpriteRendererComponent& spriteComp) {
				ImGui::ColorEdit4("Color", &spriteComp.color.x);
				ImGui::Text("Texture");

				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
						AssetMetadata metadata;
						if (AssetDatabase::GetAssetMetadata((const char*)payload->Data, metadata)) {
							if (metadata.type == AssetType::Texture2D) {
								spriteComp.texture = metadata.handle;
							}
						}
					}
					ImGui::EndDragDropTarget();
				}
			});

			DrawComponent<Rigidbody2DComponent>("Rigidbody 2D", _selectedEntt, [](Rigidbody2DComponent& rigidbody2DComp) {
				int32_t bodyTypeSelected = (int32_t)rigidbody2DComp.bodyType;
				if (DrawCombo("Body Type", bodyTypeSelected, { "Static", "Dynamic", "Kinematic" })) {
					rigidbody2DComp.bodyType = (Rigidbody2DComponent::BodyType)bodyTypeSelected;
				}

				ImGui::Checkbox("Fixed Rotation", &rigidbody2DComp.fixedRotation);
				ImGui::DragFloat("Density", &rigidbody2DComp.density, 0.1f);
				ImGui::DragFloat("Friction", &rigidbody2DComp.friction, 0.1f);
				ImGui::DragFloat("Restitution", &rigidbody2DComp.restitution, 0.1f);
				ImGui::DragFloat("Restitution Threshold", &rigidbody2DComp.restitutionThreshold, 0.1f);
			});

			DrawComponent<BoxCollider2DComponent>("Box Collider 2D", _selectedEntt, [](BoxCollider2DComponent& boxCollider2DComp) {
				ImGui::DragFloat2("Offset", glm::value_ptr(boxCollider2DComp.offset), 0.1f);
				ImGui::DragFloat2("Size", glm::value_ptr(boxCollider2DComp.size), 0.1f);
			});

			DrawComponent<CircleCollider2DComponent>("Circle Collider 2D", _selectedEntt, [](CircleCollider2DComponent& circleCollider2DComp) {
				ImGui::DragFloat2("Offset", glm::value_ptr(circleCollider2DComp.offset), 0.1f);
				ImGui::DragFloat("Radius", &circleCollider2DComp.radius, 0.1f);
			});

			DrawComponent<MonoScriptComponent>("Mono Script", _selectedEntt, [this](MonoScriptComponent& monoScriptComp) {
				char buffer[256] = { 0 };
				strcpy_s(buffer, monoScriptComp.name.c_str());
				if (ImGui::InputText("Name", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
					monoScriptComp.name = buffer;
				}

				auto obj = Scripting::GetMonoScriptObject(_selectedEntt);
				if (obj) {
					obj->GetClass()->EachPublicFields([this, &obj](std::string_view fieldName, MonoScriptClassField& field) {
						if (field.GetTypeName() == "System.Single") {
							float value = field.GetValue<float>(obj.get());
							if (ImGui::DragFloat(fieldName.data(), &value, 0.1f)) {
								field.SetValue(obj.get(), &value);
							}
						}
					});
				}
			});

			DrawComponent<TextComponent>("Text Component", _selectedEntt, [](TextComponent& textComp) {
				std::string utf8Str = Utf16ToUtf8(textComp.text);
					
				char buffer[256] = { 0 };
				strcpy_s(buffer, utf8Str.c_str());
					
				if (ImGui::InputTextMultiline("Text", buffer, sizeof(buffer), ImVec2(200, 100))) {
					textComp.text = Utf8ToUtf16(buffer);
				}

				ImGui::Text("Font");

				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
						AssetMetadata metadata;
						if (AssetDatabase::GetAssetMetadata((const char*)payload->Data, metadata)) {
							if (metadata.type == AssetType::Font) {
								textComp.font = metadata.handle;
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::ColorEdit4("Color", &textComp.color.x);
			});

			DrawComponent<SoundListenerComponent>("Sound Listener", _selectedEntt, [](SoundListenerComponent& soundListenerComp) {
				DrawVec3("Velocity", soundListenerComp.velocity);
			});

			DrawComponent<SoundSourceComponent>("Sound Source", _selectedEntt, [](SoundSourceComponent& soundSourceComp) {
				ImGui::Text("Sound");
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
						AssetMetadata metadata;
						if (AssetDatabase::GetAssetMetadata((const char*)payload->Data, metadata)) {
							if (metadata.type == AssetType::Sound) {
								soundSourceComp.sound = metadata.handle;
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::Checkbox("Loop", &soundSourceComp.loop);
				ImGui::Checkbox("Auto Play", &soundSourceComp.autoPlay);
				ImGui::DragFloat("Volume", &soundSourceComp.volume, 0.01f, 0.f, 1.f);
			});

			ImGui::Separator();

			// add component
			if (ImGui::Button("Add Component")) {
				ImGui::OpenPopup("AddComponentPopup");
			}

			if (ImGui::BeginPopup("AddComponentPopup")) {
				DrawAddComponentItem<TransformComponent>("Transform", _selectedEntt);
				DrawAddComponentItem<CameraComponent>("Camera", _selectedEntt);
				DrawAddComponentItem<SpriteRendererComponent>("Sprite Renderer", _selectedEntt);
				DrawAddComponentItem<Rigidbody2DComponent>("Rigidbody 2D", _selectedEntt);
				DrawAddComponentItem<BoxCollider2DComponent>("Box Collider 2D", _selectedEntt);
				DrawAddComponentItem<CircleCollider2DComponent>("Circle Collider 2D", _selectedEntt);
				DrawAddComponentItem<MonoScriptComponent>("Mono Script", _selectedEntt);
				DrawAddComponentItem<TextComponent>("Text", _selectedEntt);
				DrawAddComponentItem<SoundListenerComponent>("Sound Listener", _selectedEntt);
				DrawAddComponentItem<SoundSourceComponent>("Sound Source", _selectedEntt);

				ImGui::EndPopup();
			}
		}

		ImGui::End();
	}
}