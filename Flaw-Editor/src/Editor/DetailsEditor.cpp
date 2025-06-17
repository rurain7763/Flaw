#include "pch.h"
#include "DetailsEditor.h"
#include "EditorEvents.h"
#include "AssetDatabase.h"
#include "ParticleComponentDrawer.h"

namespace flaw {
	DetailsEditor::DetailsEditor(Application& app)
		: _app(app)
	{
		_app.GetEventDispatcher().Register<OnSelectEntityEvent>([this](const OnSelectEntityEvent& evn) { _selectedEntt = evn.entity; }, PID(this));
	}

	DetailsEditor::~DetailsEditor() {
		_app.GetEventDispatcher().UnregisterAll(PID(this));
	}

	void DetailsEditor::SetScene(const Ref<Scene>& scene) {
		_scene = scene;
		_selectedEntt = Entity();
	}

	void DetailsEditor::OnRender() {
		ImGui::Begin("Details");

		if (_selectedEntt) {
			auto& entityComp = _selectedEntt.GetComponent<EntityComponent>();

			ImGui::Text("ID: %d", (entt::entity)_selectedEntt);
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

			DrawComponent<RigidbodyComponent>(_selectedEntt, [](RigidbodyComponent& rigidbodyComp) {
				int32_t bodyTypeSelected = (int32_t)rigidbodyComp.bodyType;
				if (EditorHelper::DrawCombo("Body Type", bodyTypeSelected, { "Static", "Dynamic", "Kinematic" })) {
					rigidbodyComp.bodyType = (PhysicsBodyType)bodyTypeSelected;
				}

				ImGui::DragFloat("Density", &rigidbodyComp.density, 0.1f);
				ImGui::DragFloat("Static Friction", &rigidbodyComp.staticFriction, 0.1f);
				ImGui::DragFloat("Dynamic Friction", &rigidbodyComp.dynamicFriction, 0.1f);
				ImGui::DragFloat("Restitution", &rigidbodyComp.restitution, 0.1f);
			});

			DrawComponent<BoxColliderComponent>(_selectedEntt, [](BoxColliderComponent& boxColliderComp) {
				ImGui::DragFloat3("Size", glm::value_ptr(boxColliderComp.size), 0.1f);
			});

			DrawComponent<SphereColliderComponent>(_selectedEntt, [](SphereColliderComponent& sphereColliderComp) {
				ImGui::DragFloat("Radius", &sphereColliderComp.radius, 0.1f);
			});

			DrawComponent<MeshColliderComponent>(_selectedEntt, [](MeshColliderComponent& meshColliderComp) {
				// TODO: Implement MeshColliderComponent details
			});

			DrawComponent<MonoScriptComponent>(_selectedEntt, [this](MonoScriptComponent& monoScriptComp) {
				std::vector<std::string> scriptNames;
				int32_t selectedScriptIndex = -1;
				int32_t index = 0;
				for (const auto& [scriptName, scriptClass] : Scripting::GetMonoScriptDomain().GetMonoScriptClasses()) {
					if (!Scripting::IsMonoProjectComponent(scriptClass)) {
						continue; // Skip non-component scripts or non-user-defined scripts
					}
			
					scriptNames.push_back(scriptName);

					if (scriptName == monoScriptComp.name) {
						selectedScriptIndex = index;
					}

					index++;
				}

				if (EditorHelper::DrawCombo("Script", selectedScriptIndex, scriptNames)) {
					monoScriptComp.name = scriptNames[selectedScriptIndex];
				}

				if (selectedScriptIndex == -1) {
					return;
				}

				MonoScriptClass& selectedScriptClass = Scripting::GetMonoClass(monoScriptComp.name.c_str());

				auto& monoScriptSys = _scene->GetMonoScriptSystem();
				MonoScriptObject scriptObj;
				std::unordered_map<std::string, AssetHandle> assetRefs;
				std::unordered_map<std::string, UUID> componentRefs;
				std::unordered_map<std::string, UUID> entityRefs;
				if (monoScriptSys.IsMonoEntityExists(_selectedEntt.GetUUID())) {
					auto monoEntt = monoScriptSys.GetMonoEntity(_selectedEntt.GetUUID());
					scriptObj = monoEntt->GetComponent(monoScriptComp.name.c_str())->GetScriptObject();
				}
				else {
					scriptObj = MonoScriptObject(&selectedScriptClass);
					scriptObj.Instantiate();

					for (auto it = monoScriptComp.fields.begin(); it != monoScriptComp.fields.end(); ) {
						auto& filedInfo = *it;
						auto field = selectedScriptClass.GetField(filedInfo.fieldName.c_str());
						if (!field || field.GetTypeName() != filedInfo.fieldType) {
							it = monoScriptComp.fields.erase(it);
							continue;
						}

						MonoScriptClass fieldClass(scriptObj.GetMonoDomain(), field.GetMonoClass());
						if (fieldClass == Scripting::GetMonoSystemClass(MonoSystemType::Float)) {
							float value = filedInfo.As<float>();
							field.SetValue(&scriptObj, &value);
						}
						else if (fieldClass == Scripting::GetMonoSystemClass(MonoSystemType::Int32)) {
							int32_t value = filedInfo.As<int32_t>();
							field.SetValue(&scriptObj, &value);
						}
						else if (Scripting::IsMonoAsset(fieldClass)) {
							AssetHandle assetHandle = filedInfo.As<AssetHandle>();
							assetRefs[filedInfo.fieldName] = assetHandle;
						}
						else if (Scripting::IsMonoComponent(fieldClass)) {
							UUID uuid = filedInfo.As<UUID>();
							componentRefs[filedInfo.fieldName] = uuid;
						}
						else if (fieldClass == Scripting::GetMonoClass("Flaw.Entity")) {
							UUID uuid = filedInfo.As<UUID>();
							entityRefs[filedInfo.fieldName] = uuid;
						}

						it++;
					}
				}

				std::vector<MonoScriptComponent::FieldInfo> newFieldInfos;
				selectedScriptClass.EachFieldsRecursive([this, &scriptObj, &assetRefs, &componentRefs, &entityRefs, &newFieldInfos](std::string_view fieldName, MonoScriptClassField& field) {
					MonoScriptClass fieldClass(field.GetMonoDomain(), field.GetMonoClass());

					MonoScriptComponent::FieldInfo newFieldInfo;
					newFieldInfo.fieldName = fieldName.data();
					newFieldInfo.fieldType = field.GetTypeName();

					if (fieldClass == Scripting::GetMonoSystemClass(MonoSystemType::Float)) {
						float value = field.GetValue<float>(&scriptObj);
						if (EditorHelper::DrawNumericInput(fieldName.data(), value, 0.0f, 0.1f)) {
							field.SetValue(&scriptObj, &value);
						}
						newFieldInfo.SetValue(value);
					}
					else if (fieldClass == Scripting::GetMonoSystemClass(MonoSystemType::Int32)) {
						int32_t value = field.GetValue<int32_t>(&scriptObj);
						if (EditorHelper::DrawNumericInput(fieldName.data(), value, 0, 1)) {
							field.SetValue(&scriptObj, &value);
						}
						newFieldInfo.SetValue(value);
					}
					else if (fieldClass == Scripting::GetMonoAssetClass(AssetType::Prefab)) {
						auto fieldObj = MonoScriptObject(&fieldClass, field.GetValue<MonoObject*>(&scriptObj));
						auto handleField = fieldClass.GetFieldRecursive("handle");
						AssetHandle handle;
						if (fieldObj.IsValid()) {
							handle = handleField.GetValue<AssetHandle>(&fieldObj);
						}
						else {
							auto it = assetRefs.find(fieldName.data());
							if (it != assetRefs.end()) { handle = it->second; }
						}

						EditorHelper::DrawAssetPayloadTarget(fieldName.data(), handle, [&handle](const char* filepath) {
							AssetMetadata metadata;
							if (AssetDatabase::GetAssetMetadata(filepath, metadata) && metadata.type == AssetType::Prefab) {
								handle = metadata.handle;
							}
						});

						newFieldInfo.SetValue(handle);
					}
					else if (Scripting::IsMonoComponent(fieldClass)) {
						auto fieldObj = MonoScriptObject(&fieldClass, field.GetValue<MonoObject*>(&scriptObj));
						auto entityIdField = fieldClass.GetFieldRecursive("entityId");
						UUID uuid;
						if (fieldObj.IsValid()) {
							uuid = entityIdField.GetValue<UUID>(&fieldObj);
						}
						else {
							auto it = componentRefs.find(fieldName.data());
							if (it != componentRefs.end()) { 
								uuid = it->second; 
							}
						}

						auto checkComponents = [&fieldClass](Entity entity) {
							if (Scripting::IsMonoProjectComponent(fieldClass)) {
								return entity.HasComponent<MonoScriptComponent>() && entity.GetComponent<MonoScriptComponent>().name == fieldClass.GetTypeName();
							}
							else {
								return Scripting::HasEngineComponent(entity, fieldClass.GetTypeName().data());
							}
						};

						auto onDrop = [&uuid](Entity entity) { uuid = entity.GetUUID(); };

						EditorHelper::DrawEntityPayloadTarget(fieldName.data(), _scene, uuid, checkComponents, onDrop);

						newFieldInfo.SetValue(uuid);
					}
					else if (fieldClass == Scripting::GetMonoClass(Scripting::EntityClassName)) {
						auto fieldObj = MonoScriptObject(&fieldClass, field.GetValue<MonoObject*>(&scriptObj));
						auto idField = fieldClass.GetFieldRecursive("id");
						UUID uuid;
						if (fieldObj.IsValid()) {
							uuid = idField.GetValue<UUID>(&fieldObj);
						}
						else {
							auto it = entityRefs.find(fieldName.data());
							if (it != entityRefs.end()) { 
								uuid = it->second; 
							}
						}
						
						auto checkEntity = [](Entity entity) { return true; };
						auto onDrop = [&uuid](Entity entity) { uuid = entity.GetUUID(); };

						EditorHelper::DrawEntityPayloadTarget(fieldName.data(), _scene, uuid, checkEntity, onDrop);

						newFieldInfo.SetValue(uuid);
					}

					newFieldInfos.emplace_back(newFieldInfo);
				});

				monoScriptComp.fields = std::move(newFieldInfos);
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

			DrawComponent<LandscapeComponent>(_selectedEntt, [](LandscapeComponent& landScaperComp) {
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
				DrawAddComponentItem<RigidbodyComponent>(_selectedEntt);
				DrawAddComponentItem<BoxColliderComponent>(_selectedEntt);
				DrawAddComponentItem<SphereColliderComponent>(_selectedEntt);
				DrawAddComponentItem<MeshColliderComponent>(_selectedEntt);
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
				DrawAddComponentItem<LandscapeComponent>(_selectedEntt);

				ImGui::EndPopup();
			}
		}

		ImGui::End();
	}
}