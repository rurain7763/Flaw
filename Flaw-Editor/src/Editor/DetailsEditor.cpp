#include "pch.h"
#include "DetailsEditor.h"
#include "EditorEvents.h"
#include "AssetDatabase.h"
#include "ParticleComponentDrawer.h"

namespace flaw {
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
				if (EditorHelper::DrawVec3("Position", transformComp.position)) {
					transformComp.dirty = true;
				}

				vec3 degreeRotation = glm::degrees(transformComp.rotation);
				if (EditorHelper::DrawVec3("Rotation", degreeRotation, 0.f, 100.f)) {
					transformComp.dirty = true;
					transformComp.rotation = glm::radians(degreeRotation);
				}

				if (EditorHelper::DrawVec3("Scale", transformComp.scale)) {
					transformComp.dirty = true;
				}
			});

			DrawComponent<CameraComponent>("Camera", _selectedEntt, [](CameraComponent& cameraComp) {
				bool perspective = cameraComp.perspective;

				int32_t projectionSelected = perspective ? 1 : 0;
				if (EditorHelper::DrawCombo("Projection", projectionSelected, { "Orthographic", "Perspective" })) {
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

				ImGui::SameLine();

				if (ImGui::Button("-")) {
					spriteComp.texture.Invalidate();
				}
			});

			DrawComponent<Rigidbody2DComponent>("Rigidbody 2D", _selectedEntt, [](Rigidbody2DComponent& rigidbody2DComp) {
				int32_t bodyTypeSelected = (int32_t)rigidbody2DComp.bodyType;
				if (EditorHelper::DrawCombo("Body Type", bodyTypeSelected, { "Static", "Dynamic", "Kinematic" })) {
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
				EditorHelper::DrawVec3("Velocity", soundListenerComp.velocity);
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

			DrawComponent<ParticleComponent>("Particle", _selectedEntt, [this](ParticleComponent& particleComp) {
				ParticleComponentDrawer::Draw(_selectedEntt);
			});

			DrawComponent<MeshFilterComponent>("Mesh Filter", _selectedEntt, [](MeshFilterComponent& meshFilterComp) {
				// TODO: mesh filter
			});

			DrawComponent<MeshRendererComponent>("Mesh Renderer", _selectedEntt, [](MeshRendererComponent& meshRendererComp) {
				// TODO: mesh renderer
			});

			DrawComponent<SkyLightComponent>("Sky Light", _selectedEntt, [](SkyLightComponent& skyLightComp) {
				ImGui::ColorEdit3("Color", &skyLightComp.color.x);
				ImGui::DragFloat("Intensity", &skyLightComp.intensity, 0.1f);
			});

			DrawComponent<DirectionalLightComponent>("Directional Light", _selectedEntt, [](DirectionalLightComponent& directionalLightComp) {
				ImGui::ColorEdit3("Color", &directionalLightComp.color.x);
				ImGui::DragFloat("Intensity", &directionalLightComp.intensity, 0.1f);
			});

			DrawComponent<PointLightComponent>("Point Light", _selectedEntt, [](PointLightComponent& pointLightComp) {
				ImGui::ColorEdit3("Color", &pointLightComp.color.x);
				ImGui::DragFloat("Intensity", &pointLightComp.intensity, 0.1f);
				ImGui::DragFloat("Range", &pointLightComp.range, 0.1f);
			});

			DrawComponent<SpotLightComponent>("Spot Light", _selectedEntt, [](SpotLightComponent& spotLightComp) {
				ImGui::ColorEdit3("Color", &spotLightComp.color.x);
				ImGui::DragFloat("Intensity", &spotLightComp.intensity, 0.1f);
				ImGui::DragFloat("Range", &spotLightComp.range, 0.1f);
				ImGui::DragFloat("Inner", &spotLightComp.inner, 0.1f);
				ImGui::DragFloat("Outer", &spotLightComp.outer, 0.1f);
			});

			DrawComponent<SkyBoxComponent>("Sky Box", _selectedEntt, [](SkyBoxComponent& skyBoxComp) {
				ImGui::Text("Sky Box");
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
						AssetMetadata metadata;
						if (AssetDatabase::GetAssetMetadata((const char*)payload->Data, metadata)) {
							if (metadata.type == AssetType::Texture2D || metadata.type == AssetType::TextureCube) {
								skyBoxComp.texture = metadata.handle;
							}
						}
					}
					ImGui::EndDragDropTarget();
				}
			});

			DrawComponent<DecalComponent>("Decal", _selectedEntt, [](DecalComponent& decalComp) {
				ImGui::Text("Decal");
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
						AssetMetadata metadata;
						if (AssetDatabase::GetAssetMetadata((const char*)payload->Data, metadata)) {
							if (metadata.type == AssetType::Texture2D) {
								decalComp.texture = metadata.handle;
							}
						}
					}
					ImGui::EndDragDropTarget();
				}
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
				DrawAddComponentItem<ParticleComponent>("Particle", _selectedEntt);
				DrawAddComponentItem<MeshFilterComponent>("Mesh Filter", _selectedEntt);
				DrawAddComponentItem<MeshRendererComponent>("Mesh Renderer", _selectedEntt);
				DrawAddComponentItem<SkyLightComponent>("Sky Light", _selectedEntt);
				DrawAddComponentItem<DirectionalLightComponent>("Directional Light", _selectedEntt);
				DrawAddComponentItem<PointLightComponent>("Point Light", _selectedEntt);\
				DrawAddComponentItem<SpotLightComponent>("Spot Light", _selectedEntt);
				DrawAddComponentItem<SkyBoxComponent>("Sky Box", _selectedEntt);
				DrawAddComponentItem<DecalComponent>("Decal", _selectedEntt);

				ImGui::EndPopup();
			}
		}

		ImGui::End();
	}
}