#include "LandscapeEditor.h"
#include "DebugRender.h"
#include "EditorEvents.h"
#include "Editor/EditorHelper.h"
#include "AssetDatabase.h"

namespace flaw {
	constexpr uint32_t MaxTriangles = 10000;
	constexpr uint32_t MaxRayHits = MaxTriangles + 1;

	LandscapeEditor::LandscapeEditor(Application& app, EditorCamera& editorCam, ViewportEditor& viewportEditor, ContentBrowserEditor& contentEditor)
		: _app(app)
		, _editorCamera(editorCam)
		, _viewportEditor(viewportEditor)
		, _contentEditor(contentEditor)
	{
		auto& EventDispatcher = _app.GetEventDispatcher();

		// for raycasting
		_landscapeRaycastShader = Graphics::CreateComputeShader("Resources/Shaders/landscape_raycast.fx");
		_landscapeRaycastUniformCB = Graphics::CreateConstantBuffer(sizeof(LandscapeRaycastUniform));

		StructuredBuffer::Descriptor sbDesc = {};
		sbDesc.elmSize = sizeof(BVHTriangle);
		sbDesc.count = MaxTriangles;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		sbDesc.accessFlags = AccessFlag::Write;
		_landscapeRaycastTriSB = Graphics::CreateStructuredBuffer(sbDesc);

		sbDesc.elmSize = sizeof(LandscapeRayHit);
		sbDesc.count = MaxRayHits;
		sbDesc.bindFlags = BindFlag::UnorderedAccess;
		sbDesc.accessFlags = AccessFlag::Read | AccessFlag::Write;
		_landscapeRaycastResultSB = Graphics::CreateStructuredBuffer(sbDesc);

		// for draw brush
		_landscapeBrushShader = Graphics::CreateGraphicsShader("Resources/Shaders/landscape_brush.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Hull | ShaderCompileFlag::Domain | ShaderCompileFlag::Pixel);
		_landscapeBrushShader->AddInputElement<float>("POSITION", 3);
		_landscapeBrushShader->AddInputElement<float>("TEXCOORD", 2);
		_landscapeBrushShader->CreateInputLayout();
			
		_landscapeBrushUniformCB = Graphics::CreateConstantBuffer(sizeof(LandscapeBrushUniform));

		// for udpate height map
		_landscapeShader = Graphics::CreateComputeShader("Resources/Shaders/landscape_compute.fx");
		_landscapeUniformCB = Graphics::CreateConstantBuffer(sizeof(LandscapeUniform));

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
						UsageFlag::Static,
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

			if (GetBrushPos(tex2DAsset->GetTexture(), _brushPos)) {
				DrawBrush(tex2DAsset->GetTexture());

				if (Input::GetMouseButton(MouseButton::Left)) {
					UpdateLandscapeTexture(tex2DAsset->GetTexture());
				}
			}
		}
	}

	void LandscapeEditor::UpdateBrushVBAndIB() {
		// TODO: may be not necessary static
		static Entity currentEntt;

		if (!_selectedEntt || !_selectedEntt.HasComponent<LandScaperComponent>() || currentEntt == _selectedEntt) {
			return;
		}

		auto& landscapeSys = _scene->GetLandscapeSystem();
		auto& landscape = landscapeSys.GetLandscape(_selectedEntt.GetUUID());

		VertexBuffer::Descriptor vbDesc = {};
		vbDesc.usage = UsageFlag::Static;
		vbDesc.elmSize = sizeof(Vertex3D);
		vbDesc.bufferSize = landscape.mesh->vertices.size() * sizeof(Vertex3D);
		vbDesc.initialData = landscape.mesh->vertices.data();

		_landscapeBrushVB = Graphics::CreateVertexBuffer(vbDesc);

		IndexBuffer::Descriptor ibDesc = {};
		ibDesc.usage = UsageFlag::Static;
		ibDesc.bufferSize = landscape.mesh->indices.size() * sizeof(uint32_t);
		ibDesc.initialData = landscape.mesh->indices.data();

		_landscapeBrushIB = Graphics::CreateIndexBuffer(ibDesc);

		currentEntt = _selectedEntt;
	}

	void LandscapeEditor::DrawBrush(const Ref<Texture2D>& heightMapTex) {
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& mainPipeline = Graphics::GetMainGraphicsPipeline();
		auto& landscapeSys = _scene->GetLandscapeSystem();

		auto& landscapeComp = _selectedEntt.GetComponent<LandScaperComponent>();
		auto& transComp = _selectedEntt.GetComponent<TransformComponent>();
		auto& landscape = landscapeSys.GetLandscape(_selectedEntt.GetUUID());

		LandscapeBrushUniform brushUniform = {};
		brushUniform.modelMatrix = transComp.worldTransform;
		brushUniform.modelMatrix[3][1] += 0.03f; // NOTE; z-fight
		brushUniform.viewMatrix = _editorCamera.GetViewMatrix();
		brushUniform.projMatrix = _editorCamera.GetProjectionMatrix();
		brushUniform.brushType = (uint32_t)_brushType;
		brushUniform.brushPos = _brushPos;
		brushUniform.width = heightMapTex->GetWidth();
		brushUniform.height = heightMapTex->GetHeight();
		brushUniform.tilingX = landscapeComp.tilingX;
		brushUniform.tilingY = landscapeComp.tilingY;
		brushUniform.lodLvRange = vec2(0, landscapeComp.lodLevelMax);
		brushUniform.lodDistRange = landscapeComp.lodDistanceRange;
		brushUniform.cameraPos = _editorCamera.GetPosition();
		_landscapeBrushUniformCB->Update(&brushUniform, sizeof(LandscapeBrushUniform));

		UpdateBrushVBAndIB();

		cmdQueue.Begin();

		mainPipeline->SetShader(_landscapeBrushShader);
		mainPipeline->SetFillMode(FillMode::Solid);
		mainPipeline->SetBlendMode(BlendMode::Alpha, false);
		mainPipeline->SetCullMode(CullMode::Back);
		mainPipeline->SetDepthTest(DepthTest::LessEqual, false);

		cmdQueue.SetPipeline(mainPipeline);
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::ControlPoint3_PatchList);
		cmdQueue.SetConstantBuffer(_landscapeBrushUniformCB, 0);
		cmdQueue.SetTexture(heightMapTex, 0);
		cmdQueue.SetVertexBuffer(_landscapeBrushVB);
		cmdQueue.DrawIndexed(_landscapeBrushIB, _landscapeBrushIB->IndexCount());
		cmdQueue.End();

		cmdQueue.Execute();
	}

	bool LandscapeEditor::GetBrushPos(Ref<Texture2D> heightMapTex, vec2& pos) {
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& mainPipeline = Graphics::GetMainComputePipeline();
		auto& landscapeSys = _scene->GetLandscapeSystem();

		auto& landscapeComp = _selectedEntt.GetComponent<LandScaperComponent>();
		auto& transComp = _selectedEntt.GetComponent<TransformComponent>();
		auto& landscape = landscapeSys.GetLandscape(_selectedEntt.GetUUID());

		vec2 mousePos = vec2(Input::GetMouseX(), Input::GetMouseY());

		Ray ray = {};
		ray.origin = ScreenToWorld(mousePos, _viewportEditor.GetViewport(), _editorCamera.GetProjectionMatrix(), _editorCamera.GetViewMatrix());
		ray.direction = glm::normalize(ray.origin - _editorCamera.GetPosition());
		ray.length = 1000.0f;

		mat4 invTransform = glm::inverse(transComp.worldTransform);

		cmdQueue.Begin();

		LandscapeRaycastUniform landscapeRaycastUniform = {};
		landscapeRaycastUniform.rayOrigin = invTransform * vec4(ray.origin, 1.0f);
		landscapeRaycastUniform.rayDirection = invTransform * vec4(ray.direction, 0.0f);
		landscapeRaycastUniform.rayLength = ray.length;
		landscapeRaycastUniform.triangleCount = landscape.mesh->bvhTriangles.size();
		_landscapeRaycastUniformCB->Update(&landscapeRaycastUniform, sizeof(LandscapeRaycastUniform));

		_landscapeRaycastTriSB->Update(landscape.mesh->bvhTriangles.data(), landscape.mesh->bvhTriangles.size() * sizeof(BVHTriangle));

		LandscapeRayHit defaultResult = {};
		defaultResult.success = 0;
		_landscapeRaycastResultSB->Update(&defaultResult, sizeof(LandscapeRayHit));

		mainPipeline->SetShader(_landscapeRaycastShader);

		cmdQueue.SetComputePipeline(mainPipeline);
		cmdQueue.SetComputeConstantBuffer(_landscapeRaycastUniformCB, 0);
		cmdQueue.SetComputeStructuredBuffer(_landscapeRaycastTriSB, BindFlag::ShaderResource, 0);
		cmdQueue.SetComputeTexture(heightMapTex, BindFlag::ShaderResource, 1);
		cmdQueue.SetComputeStructuredBuffer(_landscapeRaycastResultSB, BindFlag::UnorderedAccess, 0);
		cmdQueue.Dispatch(CalculateDispatchGroupCount(1024, landscape.mesh->bvhTriangles.size()), 1, 1);

		cmdQueue.End();

		cmdQueue.Execute();

		std::vector<LandscapeRayHit> hits(MaxRayHits);
		_landscapeRaycastResultSB->Fetch(hits.data(), hits.size() * sizeof(LandscapeRayHit));

		int32_t resultCount = std::min(hits[0].success, MaxRayHits - 1);
		int32_t closestIndex = -1;
		float minDist = std::numeric_limits<float>::max();
		for (int32_t i = 0; i < resultCount; i++) {
			LandscapeRayHit& hit = hits[i + 1];

			if (hit.distance > minDist) {
				continue;
			}

			closestIndex = i + 1;
			minDist = hit.distance;
		}

		if (closestIndex != -1) {
			LandscapeRayHit& hit = hits[closestIndex];

			vec3 front = transComp.GetWorldFront();
			vec3 left = -transComp.GetWorldRight();

			vec3 landscapWPos = transComp.GetWorldPosition();
			vec3 landscapWScale = transComp.GetWorldScale();

			vec3 lt = landscapWPos + (front * landscapWScale.z * 0.5f) + (left * landscapWScale.x * 0.5f);
			vec3 worldHitPos = transComp.worldTransform * vec4(hit.position, 1.0f);

			pos = (vec2(worldHitPos.x, worldHitPos.z) - vec2(lt.x, lt.z)) / vec2(landscapWScale.x, -landscapWScale.z);
		}

		return closestIndex != -1;
	}

	void LandscapeEditor::UpdateLandscapeTexture(const Ref<Texture2D>& texture) {
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& mainPipeline = Graphics::GetMainComputePipeline();

		cmdQueue.Begin();

		LandscapeUniform landscapeUniform = {};
		landscapeUniform.deltaTime = Time::DeltaTime();
		landscapeUniform.brushType = (uint32_t)_brushType;
		landscapeUniform.brushPos = _brushPos;
		landscapeUniform.width = texture->GetWidth();
		landscapeUniform.height = texture->GetHeight();

		_landscapeUniformCB->Update(&landscapeUniform, sizeof(LandscapeUniform));

		mainPipeline->SetShader(_landscapeShader);

		cmdQueue.SetComputePipeline(mainPipeline);

		cmdQueue.SetComputeConstantBuffer(_landscapeUniformCB, 0);

		if (_brushType == BrushType::Texture) {
			if (_brushTextureAsset) {
				cmdQueue.SetComputeTexture(_brushTextureAsset->GetTexture(), BindFlag::ShaderResource, 0);
			}
		}
		else {
			cmdQueue.ResetTexture(0);
		}

		cmdQueue.SetComputeTexture(texture, BindFlag::UnorderedAccess, 0);
		cmdQueue.Dispatch(CalculateDispatchGroupCount(32, texture->GetWidth()), CalculateDispatchGroupCount(32, texture->GetHeight()), 1);
		cmdQueue.End();

		cmdQueue.Execute();
	}
}