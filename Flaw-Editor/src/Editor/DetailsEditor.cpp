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

			DrawComponent<TransformComponent>(_selectedEntt, [](TransformComponent& transformComp) {
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

			DrawComponent<CameraComponent>(_selectedEntt, [](CameraComponent& cameraComp) {
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
					ImGui::DragFloat("Otho Size", &cameraComp.orthoSize, 0.1f);
				}

				ImGui::DragFloat("Aspect Ratio", &cameraComp.aspectRatio, 0.1f);
				ImGui::DragFloat("Near Clip", &cameraComp.nearClip, 0.1f);
				ImGui::DragFloat("Far Clip", &cameraComp.farClip, 0.1f);
				});

			DrawComponent<SpriteRendererComponent>(_selectedEntt, [this](SpriteRendererComponent& spriteComp) {
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

			DrawComponent<Rigidbody2DComponent>(_selectedEntt, [](Rigidbody2DComponent& rigidbody2DComp) {
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

			DrawComponent<BoxCollider2DComponent>(_selectedEntt, [](BoxCollider2DComponent& boxCollider2DComp) {
				ImGui::DragFloat2("Offset", glm::value_ptr(boxCollider2DComp.offset), 0.1f);
				ImGui::DragFloat2("Size", glm::value_ptr(boxCollider2DComp.size), 0.1f);
				});

			DrawComponent<CircleCollider2DComponent>(_selectedEntt, [](CircleCollider2DComponent& circleCollider2DComp) {
				ImGui::DragFloat2("Offset", glm::value_ptr(circleCollider2DComp.offset), 0.1f);
				ImGui::DragFloat("Radius", &circleCollider2DComp.radius, 0.1f);
				});

			DrawComponent<MonoScriptComponent>(_selectedEntt, [this](MonoScriptComponent& monoScriptComp) {
				std::vector<std::string> scriptNames;
				int32_t selectedScriptIndex = -1;
				int32_t index = 0;
				for (const auto& [scriptName, scriptClass] : Scripting::GetMonoScriptDomain().GetMonoScriptClasses()) {
					if (Scripting::IsEngineComponent(scriptClass->GetReflectionType())) {
						continue; // Skip engine components
					}
					
					scriptNames.push_back(scriptName);

					if (scriptName == monoScriptComp.name) {
						selectedScriptIndex = index;
					}

					index++;
				}

				if (EditorHelper::DrawCombo("Script", selectedScriptIndex, scriptNames)) {
					Scripting::DestroyTempMonoScriptObject(_selectedEntt.GetUUID());
					monoScriptComp.name = scriptNames[selectedScriptIndex];
				}

				if (selectedScriptIndex == -1) {
					return;
				}

				auto tempMonoScriptObject = Scripting::GetTempMonoScriptObject(_selectedEntt.GetUUID());
				if (!tempMonoScriptObject) {
					tempMonoScriptObject = Scripting::CreateTempMonoScriptObject(_selectedEntt.GetUUID(), monoScriptComp.name.c_str());
				}

				tempMonoScriptObject->GetClass()->EachPublicFields([&tempMonoScriptObject](std::string_view fieldName, MonoScriptClassField& field) {
					auto typeName = field.GetTypeName();
					
					if (typeName == "System.Single") {
						float value = 0.f;
						field.GetValue(tempMonoScriptObject.get(), &value);
						if (ImGui::DragFloat(fieldName.data(), &value, 0.1f)) {
							field.SetValue(tempMonoScriptObject.get(), &value);
						}
					}
					else if (typeName == "System.Int32") {
						int32_t value = 0;
						field.GetValue(tempMonoScriptObject.get(), &value);
						if (ImGui::DragInt(fieldName.data(), &value, 1)) {
							field.SetValue(tempMonoScriptObject.get(), &value);
						}
					}
				});
			});

			DrawComponent<TextComponent>(_selectedEntt, [](TextComponent& textComp) {
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

			DrawComponent<SoundListenerComponent>(_selectedEntt, [](SoundListenerComponent& soundListenerComp) {
				EditorHelper::DrawVec3("Velocity", soundListenerComp.velocity);
				});

			DrawComponent<SoundSourceComponent>(_selectedEntt, [](SoundSourceComponent& soundSourceComp) {
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

			DrawComponent<ParticleComponent>(_selectedEntt, [this](ParticleComponent& particleComp) {
				ParticleComponentDrawer::Draw(_selectedEntt);
				});

			DrawComponent<StaticMeshComponent>(_selectedEntt, [](StaticMeshComponent& staticMeshComp) {
				ImGui::Text("Mesh");
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
						AssetMetadata metadata;
						if (AssetDatabase::GetAssetMetadata((const char*)payload->Data, metadata)) {
							if (metadata.type == AssetType::StaticMesh) {
								auto asset = AssetManager::GetAsset<StaticMeshAsset>(metadata.handle);
								staticMeshComp.mesh = metadata.handle;
								staticMeshComp.materials = asset->GetMaterialHandles();
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				if (ImGui::CollapsingHeader("Materials")) {
					for (size_t i = 0; i < staticMeshComp.materials.size(); ++i) {
						if (AssetManager::IsAssetRegistered(staticMeshComp.materials[i])) {
							ImGui::Text("Material %zu", i);
						}
						else {
							ImGui::Text("Material Invalid##%d", i);
						}
						if (ImGui::BeginDragDropTarget()) {
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
								AssetMetadata metadata;
								if (AssetDatabase::GetAssetMetadata((const char*)payload->Data, metadata)) {
									if (metadata.type == AssetType::Material) {
										staticMeshComp.materials[i] = metadata.handle;
									}
								}
							}
							ImGui::EndDragDropTarget();
						}
					}
				}

				ImGui::Checkbox("Cast Shadow", &staticMeshComp.castShadow);
			});

			DrawComponent<SkeletalMeshComponent>(_selectedEntt, [](SkeletalMeshComponent& skeletalMeshComp) {
				ImGui::Text("Mesh");
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
						AssetMetadata metadata;
						if (AssetDatabase::GetAssetMetadata((const char*)payload->Data, metadata)) {
							if (metadata.type == AssetType::SkeletalMesh) {
								auto asset = AssetManager::GetAsset<SkeletalMeshAsset>(metadata.handle);
								skeletalMeshComp.mesh = metadata.handle;
								skeletalMeshComp.materials = asset->GetMaterialHandles();
								skeletalMeshComp.skeleton = asset->GetSkeletonHandle();
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				if (ImGui::CollapsingHeader("Materials")) {
					for (size_t i = 0; i < skeletalMeshComp.materials.size(); ++i) {
						if (AssetManager::IsAssetRegistered(skeletalMeshComp.materials[i])) {
							ImGui::Text("Material %zu", i);
						}
						else {
							ImGui::Text("Material Invalid##%d", i);
						}
						
						if (ImGui::BeginDragDropTarget()) {
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
								AssetMetadata metadata;
								if (AssetDatabase::GetAssetMetadata((const char*)payload->Data, metadata)) {
									if (metadata.type == AssetType::Material) {
										skeletalMeshComp.materials[i] = metadata.handle;
									}
								}
							}
							ImGui::EndDragDropTarget();
						}
					}
				}

				if (AssetManager::IsAssetRegistered(skeletalMeshComp.skeleton)) {
					ImGui::Text("Skeleton");
				}
				else {
					ImGui::Text("Skeleton Invalid");
				}
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
						AssetMetadata metadata;
						if (AssetDatabase::GetAssetMetadata((const char*)payload->Data, metadata)) {
							if (metadata.type == AssetType::SkeletalMesh) {
								skeletalMeshComp.skeleton = metadata.handle;
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				// TODO: 임시 테스트
				ImGui::Text("Blend Factor");
				if (ImGui::DragFloat("Blend Factor", &skeletalMeshComp.blendFactor, 0.1f, 0.0, 1.0)) {
					skeletalMeshComp.blendFactor = glm::clamp(skeletalMeshComp.blendFactor, 0.f, 1.f);
				}

				ImGui::Checkbox("Cast Shadow", &skeletalMeshComp.castShadow);
			});

			DrawComponent<SkyLightComponent>(_selectedEntt, [](SkyLightComponent& skyLightComp) {
				ImGui::ColorEdit3("Color", &skyLightComp.color.x);
				ImGui::DragFloat("Intensity", &skyLightComp.intensity, 0.1f);
				});

			DrawComponent<DirectionalLightComponent>(_selectedEntt, [](DirectionalLightComponent& directionalLightComp) {
				ImGui::ColorEdit3("Color", &directionalLightComp.color.x);
				ImGui::DragFloat("Intensity", &directionalLightComp.intensity, 0.1f);
				});

			DrawComponent<PointLightComponent>(_selectedEntt, [](PointLightComponent& pointLightComp) {
				ImGui::ColorEdit3("Color", &pointLightComp.color.x);
				ImGui::DragFloat("Intensity", &pointLightComp.intensity, 0.1f);
				ImGui::DragFloat("Range", &pointLightComp.range, 0.1f);
				});

			DrawComponent<SpotLightComponent>(_selectedEntt, [](SpotLightComponent& spotLightComp) {
				ImGui::ColorEdit3("Color", &spotLightComp.color.x);
				ImGui::DragFloat("Intensity", &spotLightComp.intensity, 0.1f);
				ImGui::DragFloat("Range", &spotLightComp.range, 0.1f);
				ImGui::DragFloat("Inner", &spotLightComp.inner, 0.1f);
				ImGui::DragFloat("Outer", &spotLightComp.outer, 0.1f);
				});

			DrawComponent<SkyBoxComponent>(_selectedEntt, [](SkyBoxComponent& skyBoxComp) {
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

			DrawComponent<DecalComponent>(_selectedEntt, [](DecalComponent& decalComp) {
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

			DrawComponent<LandScaperComponent>(_selectedEntt, [](LandScaperComponent& landScaperComp) {
				ImGui::Text("Height Map");
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
						AssetMetadata metadata;
						if (AssetDatabase::GetAssetMetadata((const char*)payload->Data, metadata)) {
							if (metadata.type == AssetType::Texture2D) {
								landScaperComp.heightMap = metadata.handle;
								landScaperComp.dirty = true;
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::SameLine();

				if (ImGui::Button("-##Height")) {
					landScaperComp.heightMap.Invalidate();
					landScaperComp.dirty = true;
				}

				ImGui::Text("Albedo Texture2D Array");
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
						AssetMetadata metadata;
						if (AssetDatabase::GetAssetMetadata((const char*)payload->Data, metadata)) {
							if (metadata.type == AssetType::Texture2DArray) {
								landScaperComp.albedoTexture2DArray = metadata.handle;
								landScaperComp.dirty = true;
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::SameLine();

				if (ImGui::Button("-##Albedo")) {
					landScaperComp.albedoTexture2DArray.Invalidate();
					landScaperComp.dirty = true;
				}

				if (ImGui::DragInt("Tiling X", (int32_t*)&landScaperComp.tilingX, 1, 1)) {
					landScaperComp.dirty = true;
				}

				if (ImGui::DragInt("Tiling Y", (int32_t*)&landScaperComp.tilingY, 1, 1)) {
					landScaperComp.dirty = true;
				}

				if (ImGui::DragInt("LOD Level Max", (int32_t*)&landScaperComp.lodLevelMax, 1, 1)) {
					landScaperComp.dirty = true;
				}

				if (ImGui::DragFloat2("LOD Distance Range", glm::value_ptr(landScaperComp.lodDistanceRange), 0.1f)) {
					landScaperComp.dirty = true;
				}
			});

			ImGui::Separator();

			// add component
			if (ImGui::Button("Add Component")) {
				ImGui::OpenPopup("AddComponentPopup");
			}

			if (ImGui::BeginPopup("AddComponentPopup")) {
				DrawAddComponentItem<TransformComponent>(_selectedEntt);
				DrawAddComponentItem<CameraComponent>(_selectedEntt);
				DrawAddComponentItem<SpriteRendererComponent>(_selectedEntt);
				DrawAddComponentItem<Rigidbody2DComponent>(_selectedEntt);
				DrawAddComponentItem<BoxCollider2DComponent>(_selectedEntt);
				DrawAddComponentItem<CircleCollider2DComponent>(_selectedEntt);
				DrawAddComponentItem<MonoScriptComponent>(_selectedEntt);
				DrawAddComponentItem<TextComponent>(_selectedEntt);
				DrawAddComponentItem<SoundListenerComponent>(_selectedEntt);
				DrawAddComponentItem<SoundSourceComponent>(_selectedEntt);
				DrawAddComponentItem<ParticleComponent>(_selectedEntt);
				DrawAddComponentItem<StaticMeshComponent>(_selectedEntt);
				DrawAddComponentItem<SkeletalMeshComponent>(_selectedEntt);
				DrawAddComponentItem<SkyLightComponent>(_selectedEntt);
				DrawAddComponentItem<DirectionalLightComponent>(_selectedEntt);
				DrawAddComponentItem<PointLightComponent>(_selectedEntt);
				DrawAddComponentItem<SpotLightComponent>(_selectedEntt);
				DrawAddComponentItem<SkyBoxComponent>(_selectedEntt);
				DrawAddComponentItem<DecalComponent>(_selectedEntt);
				DrawAddComponentItem<LandScaperComponent>(_selectedEntt);

				ImGui::EndPopup();
			}
		}

		ImGui::End();
	}
}