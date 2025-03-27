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
		_eventDispatcher.Register<OnSelectEntityEvent>([this](const OnSelectEntityEvent& evn) {
			_selectedEntt = evn.entity;
		}, PID(this));
	}

	OutlinerEditor::~OutlinerEditor() {
		_eventDispatcher.UnregisterAll(PID(this));
	}

	void OutlinerEditor::SetScene(const Ref<Scene>& scene) {
		_scene = scene;
		_selectedEntt = Entity();
	}

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

		// TODO: this code must be some where else
		ImGui::Begin("Details");

		if (_selectedEntt) {
			auto& entityComp = _selectedEntt.GetComponent<EntityComponent>();

			ImGui::Text("ID: %d", (uint32_t)_selectedEntt);
			
			char nameBuffer[256] = { 0 };
			strcpy_s(nameBuffer, entityComp.name.c_str());
			if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
				entityComp.name = nameBuffer;
			}

			if (_selectedEntt.HasComponent<TransformComponent>()) {
				ImGui::Separator();

				DrawComponent<TransformComponent>("Transform", _selectedEntt, [](TransformComponent& transformComp) {
					DrawVec3("Position", transformComp.position);

					vec3 degreeRotation = glm::degrees(transformComp.rotation);
					if (DrawVec3("Rotation", degreeRotation, 0.f, 100.f)) {
						transformComp.rotation = glm::radians(degreeRotation);
					}

					DrawVec3("Scale", transformComp.scale);
				});
			}

			if (_selectedEntt.HasComponent<CameraComponent>()) {
				ImGui::Separator();

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
			}

			if (_selectedEntt.HasComponent<SpriteRendererComponent>()) {
				ImGui::Separator();

				DrawComponent<SpriteRendererComponent>("Sprite Renderer", _selectedEntt, [this](SpriteRendererComponent& spriteComp) {
					ImGui::ColorEdit4("Color", &spriteComp.color.x);
					ImGui::Text("Texture");
					if (ImGui::BeginDragDropTarget()) {
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
							std::filesystem::path filePath = (const char*)payload->Data;
							
							if (filePath.extension() == ".png" 
								|| filePath.extension() == ".jpg" 
								|| filePath.extension() == ".jpeg") 
							{
								Image img(filePath.generic_string().c_str());
							
								Texture::Descriptor desc = {};
								desc.width = img.Width();
								desc.height = img.Height();

								PixelFormat format = PixelFormat::RGBA8;
								switch (img.Channels())
								{
								case 1:
									format = PixelFormat::R8;
									break;
								case 2:
									format = PixelFormat::RG8;
									break;
								case 3:
									format = PixelFormat::RGB8;
									break;
								case 4:
									format = PixelFormat::RGBA8;
									break;
								}

								desc.format = format;
								desc.data = img.Data().data();
								desc.wrapS = Texture::Wrap::ClampToEdge;
								desc.wrapT = Texture::Wrap::ClampToEdge;
								desc.minFilter = Texture::Filter::Linear;
								desc.magFilter = Texture::Filter::Linear;

								desc.usage = UsageFlag::Static;
								desc.bindFlags = BindFlag::ShaderResource;

								spriteComp.texture = _app.GetGraphicsContext().CreateTexture2D(desc);
							}
						}
						ImGui::EndDragDropTarget();
					}
				});
			}

			if (_selectedEntt.HasComponent<Rigidbody2DComponent>()) {
				ImGui::Separator();
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
			}

			if (_selectedEntt.HasComponent<BoxCollider2DComponent>()) {
				ImGui::Separator();
				DrawComponent<BoxCollider2DComponent>("Box Collider 2D", _selectedEntt, [](BoxCollider2DComponent& boxCollider2DComp) {
					ImGui::DragFloat2("Offset", glm::value_ptr(boxCollider2DComp.offset), 0.1f);
					ImGui::DragFloat2("Size", glm::value_ptr(boxCollider2DComp.size), 0.1f);
				});
			}

			if (_selectedEntt.HasComponent<MonoScriptComponent>()) {
				ImGui::Separator();
				DrawComponent<MonoScriptComponent>("Mono Script", _selectedEntt, [this](MonoScriptComponent& monoScriptComp) {
					char buffer[256] = { 0 };
					strcpy_s(buffer, monoScriptComp.name.c_str());
					if (ImGui::InputText("Name", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
						monoScriptComp.name = buffer;
					}

					auto obj = Scripting::GetMonoScriptObject(_selectedEntt);
					if (obj) {
						obj->GetClass()->EachFields([this, &obj](std::string_view fieldName, MonoScriptClassField& field) {
							if (field.GetTypeName() == "System.Single") {
								float value = field.GetValue<float>(obj.get());
								if (ImGui::DragFloat(fieldName.data(), &value, 0.1f)) {
									field.SetValue(obj.get(), &value);
								}
							}
						});
					}
				});
			}

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
				DrawAddComponentItem<MonoScriptComponent>("Mono Script", _selectedEntt);

				ImGui::EndPopup();
			}
		}

		ImGui::End();
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