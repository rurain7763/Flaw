#include "LandscapeEditor.h"
#include "DebugRender.h"
#include "EditorEvents.h"
#include "Editor/EditorHelper.h"
#include "AssetDatabase.h"

namespace flaw {
	LandscapeEditor::LandscapeEditor(Application& app, EditorCamera& editorCam, ViewportEditor& viewportEditor, ContentBrowserEditor& contentEditor)
		: _app(app)
		, _editorCamera(editorCam)
		, _viewportEditor(viewportEditor)
		, _contentEditor(contentEditor)
	{
		auto& EventDispatcher = _app.GetEventDispatcher();

		_landscapeUniformCB = Graphics::CreateConstantBuffer(sizeof(LandscapeUniform));

		_landscapeShader = Graphics::CreateComputeShader("Resources/Shaders/landscape_compute.fx");

		_landscapePipeline = Graphics::CreateComputePipeline();
		_landscapePipeline->SetShader(_landscapeShader);

		EventDispatcher.Register<OnSelectEntityEvent>([this](const OnSelectEntityEvent& evn) { _selectedEntt = evn.entity; }, PID(this));
	}

	LandscapeEditor::~LandscapeEditor() {
		_app.GetEventDispatcher().UnregisterAll(PID(this));
	}

	void LandscapeEditor::SetScene(const Ref<Scene>& scene) {
		_scene = scene;
		_selectedEntt = Entity();
	}

	void LandscapeEditor::OnRender() {
		if (!_scene) {
			return;
		}

		ImGui::Begin("Landscape");
		Finalizer finalizer([]() { ImGui::End(); });

		if (!_selectedEntt || !_selectedEntt.HasComponent<LandScaperComponent>()) {
			return;
		}

		auto& landscapeComp = _selectedEntt.GetComponent<LandScaperComponent>();

		auto tex2DAsset = AssetManager::GetAsset<Texture2DAsset>(landscapeComp.heightMap);
		if (!tex2DAsset) {
			static int32_t width = 1024;
			static int32_t height = 1024;

			ImGui::InputInt("Width", &width);
			ImGui::InputInt("Height", &height);

			if (ImGui::Button("Create Height Map")) {
				// Create empty texture
				TextureCreateSettings settings;
				// TODO: how can we create proper name?
				settings.destPath = _contentEditor.GetCurrentDir() + "/LandscapeHeightMap.asset";
				settings.writeArchiveFunc = [](SerializationArchive& archive) {
					Texture2DAsset::WriteToArchive(
						PixelFormat::R32F,
						width,
						height,
						0,
						BindFlag::ShaderResource | BindFlag::UnorderedAccess,
						std::vector<uint8_t>(width * height * GetSizePerPixel(PixelFormat::R32F)),
						archive
					);
				};
				settings.textureType = TextureType::Texture2D;

				AssetDatabase::CreateAsset(&settings);
			}
		}
		else if (tex2DAsset->GetTexture()->GetBindFlags() & BindFlag::UnorderedAccess) {
			std::vector<std::string> brushTypes = { "Round", "Texture" };

			int32_t brushType = (int32_t)_brushType;
			if (EditorHelper::DrawCombo("Brush Type", brushType, brushTypes)) {
				_brushType = (BrushType)brushType;
			}

			if (_brushType == BrushType::Texture) {
				ImGui::Text("Brush Texture");
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
						AssetMetadata metadata;
						if (AssetDatabase::GetAssetMetadata((const char*)payload->Data, metadata)) {
							if (metadata.type == AssetType::Texture2D) {
								_brushTextureAsset = AssetManager::GetAsset<Texture2DAsset>(metadata.handle);
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::SameLine();

				if (ImGui::Button("-")) {
					if (_brushTextureAsset) {
						_brushTextureAsset->Unload();
						_brushTextureAsset.reset();
					}
				}
			}

			if (GetBrushPos(_brushPos) && Input::GetMouseButton(MouseButton::Left)) {
				UpdateLandscapeTexture(tex2DAsset->GetTexture());
			}
		}
	}

	bool LandscapeEditor::GetBrushPos(vec2& pos) {
		auto& landscapeSys = _scene->GetLandscapeSystem();
		auto& enttComp = _selectedEntt.GetComponent<EntityComponent>();
		auto& landscapeComp = _selectedEntt.GetComponent<LandScaperComponent>();
		auto& transComp = _selectedEntt.GetComponent<TransformComponent>();

		vec2 mousePos = vec2(Input::GetMouseX(), Input::GetMouseY());

		Ray ray = {};
		ray.origin = ScreenToWorld(mousePos, _viewportEditor.GetViewport(), _editorCamera.GetProjectionMatrix(), _editorCamera.GetViewMatrix());
		ray.direction = glm::normalize(ray.origin - _editorCamera.GetPosition());
		ray.length = 1000.0f;

		mat4 invTransform = glm::inverse(transComp.worldTransform);

		ray.origin = invTransform * vec4(ray.origin, 1.0f);
		ray.direction = invTransform * vec4(ray.direction, 0.0f);

		auto& landscape = landscapeSys.GetLandscape(enttComp.uuid);

		RayHit hit = {};
		if (Raycast::RaycastBVH(landscape.mesh->bvhNodes, landscape.mesh->bvhTriangles, ray, hit)) {
			vec3 front = transComp.GetWorldFront();
			vec3 left = -transComp.GetWorldRight();

			vec3 landscapWPos = transComp.GetWorldPosition();
			vec3 landscapWScale = transComp.GetWorldScale();

			vec3 lt = landscapWPos + (front * landscapWScale.z * 0.5f) + (left * landscapWScale.x * 0.5f);
			vec3 worldHitPos = transComp.worldTransform * vec4(hit.position, 1.0f);

			pos = (vec2(worldHitPos.x, worldHitPos.z) - vec2(lt.x, lt.z)) / vec2(landscapWScale.x, -landscapWScale.z);
			return true;
		}

		return false;
	}

	void LandscapeEditor::UpdateLandscapeTexture(const Ref<Texture2D>& texture) {
		auto& cmdQueue = Graphics::GetCommandQueue();

		cmdQueue.Begin();

		LandscapeUniform landscapeUniform = {};
		landscapeUniform.deltaTime = Time::DeltaTime();
		landscapeUniform.brushType = (uint32_t)_brushType;
		landscapeUniform.brushPos = _brushPos;
		landscapeUniform.width = texture->GetWidth();
		landscapeUniform.height = texture->GetHeight();

		_landscapeUniformCB->Update(&landscapeUniform, sizeof(LandscapeUniform));

		cmdQueue.SetComputeConstantBuffer(_landscapeUniformCB, 0);
		cmdQueue.SetComputePipeline(_landscapePipeline);

		if (_brushType == BrushType::Texture) {
			if (_brushTextureAsset) {
				cmdQueue.SetComputeTexture(_brushTextureAsset->GetTexture(), BindFlag::ShaderResource, 0);
			}
		}
		else {
			cmdQueue.ResetTexture(0);
		}

		cmdQueue.SetComputeTexture(texture, BindFlag::UnorderedAccess, 0);
		cmdQueue.Dispatch(
			CalculateDispatchGroupCount(32, texture->GetWidth()), 
			CalculateDispatchGroupCount(32, texture->GetHeight()), 
			1
		);
		cmdQueue.End();

		cmdQueue.Execute();
	}
}