#include "ViewportEditor.h"
#include "EditorEvents.h"
#include "DebugRender.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "Editor/ImGuizmo.h"
#include <limits>

namespace flaw {
    ViewportEditor::ViewportEditor(Application& app, EditorCamera& camera)
        : _platformContext(Platform::GetPlatformContext())
        , _graphicsContext(Graphics::GetGraphicsContext())
        , _eventDispatcher(app.GetEventDispatcher())
        , _editorCamera(camera)
		, _useEditorCamera(true)
    {
        CreateRequiredTextures();

        _mvpConstantBuffer = _graphicsContext.CreateConstantBuffer(sizeof(MVPMatrices));

		Ref<GraphicsShader> shader = _graphicsContext.CreateGraphicsShader("Resources/Shaders/std3d_outline.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
        shader->AddInputElement<float>("POSITION", 3);
		shader->AddInputElement<float>("NORMAL", 3);
        shader->CreateInputLayout();
        
        _outlineGraphicsPipeline = _graphicsContext.CreateGraphicsPipeline();
		_outlineGraphicsPipeline->SetShader(shader);
		_outlineGraphicsPipeline->SetBlendMode(BlendMode::Default);
        _outlineGraphicsPipeline->SetDepthTest(DepthTest::Less, false);
		_outlineGraphicsPipeline->SetCullMode(CullMode::Front);

        _eventDispatcher.Register<OnSelectEntityEvent>([this](const OnSelectEntityEvent& evn) { _selectedEntt = evn.entity; }, PID(this));
        _eventDispatcher.Register<WindowResizeEvent>([this](const WindowResizeEvent& evn) { CreateRequiredTextures(); }, PID(this));
		_eventDispatcher.Register<OnSceneStateChangeEvent>([this](const OnSceneStateChangeEvent& evn) { _useEditorCamera = evn.state == SceneState::Edit; }, PID(this));
		_eventDispatcher.Register<OnScenePauseEvent>([this](const OnScenePauseEvent& evn) { _useEditorCamera = evn.pause; }, PID(this));
    }

	ViewportEditor::~ViewportEditor() {
		_eventDispatcher.UnregisterAll(PID(this));
	}

    void ViewportEditor::SetScene(const Ref<Scene>& scene) {
        _scene = scene;
        _selectedEntt = Entity();
    }

    void ViewportEditor::OnRender() {
        if (!_scene) {
            return;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar);

        ImVec2 currentPos = ImGui::GetCursorScreenPos();
        ImVec2 currentSize = ImGui::GetContentRegionAvail();
        vec2 relativePos = vec2(currentPos.x - _platformContext.GetX(), currentPos.y - _platformContext.GetY());

		_viewport = vec4(relativePos.x, relativePos.y, currentSize.x, currentSize.y);

        // 에디터 카메라 및 모든 scene 내의 카메라 aspect ratio 업데이트
        const float aspectRatio = currentSize.x / currentSize.y;

        mat4 viewMatrix = mat4(1.0f);
        mat4 projectionMatrix = mat4(1.0f);
        bool isPerspective = true;

        if (_useEditorCamera) {
            _editorCamera.SetAspectRatio(aspectRatio);

            viewMatrix = _editorCamera.GetViewMatrix();
            projectionMatrix = _editorCamera.GetProjectionMatrix();
            isPerspective = _editorCamera.IsPerspective();
        }
        else {
            for (auto&& [entity, transComp, cameraComp] : _scene->GetRegistry().view<TransformComponent, CameraComponent>().each()) {
                cameraComp.aspectRatio = aspectRatio;

                if (cameraComp.depth == 0) {
                    viewMatrix = ViewMatrix(transComp.position, transComp.rotation);
                    projectionMatrix = cameraComp.GetProjectionMatrix();
                    isPerspective = cameraComp.perspective;
                }
            }
        }

        if (_selectedEntt) {
            DrawOulineOfSelectedEntity(viewMatrix, projectionMatrix);
            DrawDebugComponent();
        }

        Renderer2D::End();

		auto renderTargetTex = Graphics::GetMainRenderPass()->GetRenderTargetTex(0);
        renderTargetTex->CopyTo(_captureRenderTargetTexture);

        auto dxTexture = std::static_pointer_cast<DXTexture2D>(_captureRenderTargetTexture);

#if false
        // MEMO: 마우스 피킹 ray 테스트
        if (ImGui::IsWindowHovered() && !ImGuizmo::IsOver() && Input::GetMouseButtonDown(MouseButton::Left)) {
            MousePickingWithRay(mousePos, relativePos, vec2(currentSize.x, currentSize.y));
        }
#else
        // NOTE: 마우스 피킹 render target 테스트
		vec2 mousePos = vec2(Input::GetMouseX(), Input::GetMouseY());

        vec2 remap = Remap(
            vec2(0.0),
            vec2(currentSize.x, currentSize.y),
            mousePos - relativePos,
            vec2(0.0),
            vec2(dxTexture->GetWidth(), dxTexture->GetHeight())
        );

        if (ImGui::IsWindowHovered() && !ImGuizmo::IsOver() && Input::GetMouseButtonDown(MouseButton::Left)) {
            uint32_t id = MousePicking(remap.x, remap.y);

			_selectedEntt = Entity();
            for (auto&& [entity] : _scene->GetRegistry().view<entt::entity>().each()) {
                if ((uint32_t)entity == id) {
                    _selectedEntt = Entity(entity, _scene.get());
                    break;
                }
            }

			_eventDispatcher.Dispatch<OnSelectEntityEvent>(_selectedEntt);
        }
#endif
        
        ImGui::Image((ImTextureID)dxTexture->GetShaderResourceView().Get(), currentSize);

        // NOTE: 기즈모 드로우
        if (_selectedEntt) {
            ImGuizmo::SetOrthographic(!isPerspective);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(currentPos.x, currentPos.y, currentSize.x, currentSize.y);

            auto& enttTransComp = _selectedEntt.GetComponent<TransformComponent>();
            mat4 enttTransform = enttTransComp.worldTransform;

            static ImGuizmo::OPERATION op = ImGuizmo::OPERATION::TRANSLATE;

            bool snapping = Input::GetKey(KeyCode::LCtrl);
            float snapValue = 0.5f;
            if (snapping && op == ImGuizmo::OPERATION::ROTATE) {
                snapValue = 45.0f;
            }

            if (!_editorCamera.IsMoving() && ImGui::IsWindowHovered()) {
                if (Input::GetKeyDown(KeyCode::W)) {
                    op = ImGuizmo::OPERATION::TRANSLATE;
                }

                if (Input::GetKeyDown(KeyCode::E)) {
                    op = ImGuizmo::OPERATION::ROTATE;
                }

                if (Input::GetKeyDown(KeyCode::R)) {
                    op = ImGuizmo::OPERATION::SCALE;
                }
            }

            ImGuizmo::Manipulate(
                glm::value_ptr(viewMatrix),
                glm::value_ptr(projectionMatrix),
                op,
                ImGuizmo::MODE::LOCAL,
                glm::value_ptr(enttTransform),
                nullptr,
                snapping ? glm::value_ptr(glm::vec3(snapValue, snapValue, snapValue)) : nullptr
            );

            if (ImGuizmo::IsUsing()) {
                enttTransComp.dirty = true;

                if (_selectedEntt.HasParent()) {
					auto& parentTransComp = _selectedEntt.GetParent().GetComponent<TransformComponent>();

					mat4 localMat = glm::inverse(parentTransComp.worldTransform) * enttTransform;
					ExtractModelMatrix(localMat, enttTransComp.position, enttTransComp.rotation, enttTransComp.scale);
				}
                else {
                    ExtractModelMatrix(enttTransform, enttTransComp.position, enttTransComp.rotation, enttTransComp.scale);
                }
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
	}

    void ViewportEditor::CreateRequiredTextures() {
        Texture2D::Descriptor desc = {};

		int32_t frameBufferWidth, frameBufferHeight;
		_platformContext.GetFrameBufferSize(frameBufferWidth, frameBufferHeight);
        desc.width = frameBufferWidth;
        desc.height = frameBufferHeight;
        desc.format = PixelFormat::RGBA8;
        desc.usage = UsageFlag::Static;
        desc.bindFlags = BindFlag::ShaderResource;

        _captureRenderTargetTexture = _graphicsContext.CreateTexture2D(desc);
    }

    uint32_t ViewportEditor::MousePicking(int32_t x, int32_t y) {
		auto mainMrt = Graphics::GetMainRenderPass();
		auto idRenderTargetTex = mainMrt->GetRenderTargetTex(1);

		if (x < 0 || x >= idRenderTargetTex->GetWidth() || y < 0 || y >= idRenderTargetTex->GetHeight()) {
            return std::numeric_limits<int32_t>().max();
		}

        Texture2D::Descriptor desc = {};
		desc.width = 1;
		desc.height = 1;
		desc.format = PixelFormat::R32_UINT;
		desc.usage = UsageFlag::Staging;
		desc.access = AccessFlag::Read;

		Ref<Texture2D> stagingTexture = _graphicsContext.CreateTexture2D(desc);

        idRenderTargetTex->CopyToSub(stagingTexture, x, y, 1, 1);

        uint32_t pixel;
		stagingTexture->Fetch(&pixel, sizeof(uint32_t));

		return pixel;
	}

    uint32_t ViewportEditor::MousePickingWithRay(const vec2& mousePos, const vec2& relativePos, const vec2& currentSize) {
        vec3 screenToWorld = ScreenToWorld(
            mousePos,
            vec4(relativePos.x, relativePos.y, currentSize.x, currentSize.y),
            _editorCamera.GetProjectionMatrix(),
            _editorCamera.GetViewMatrix()
        );

        vec3 rayDirection = glm::normalize(screenToWorld - _editorCamera.GetPosition());

        const std::vector<vec3> vertices = {
            { -0.5f, 0.5f, 0.0f },
            { 0.5f, 0.5f, 0.0f },
            { 0.5f, -0.5f, 0.0f },
            { -0.5f, -0.5f, 0.0f }
        };

		uint32_t candidateEnttId = std::numeric_limits<int32_t>().max();
        float minDistance = std::numeric_limits<float>().max();
        for (auto&& [entity, enttComp, transform, sprite] : _scene->GetRegistry().view<EntityComponent, TransformComponent, SpriteRendererComponent>().each()) {
            const mat4 transMat = transform.worldTransform;

            std::vector<vec3> worldVertices = {
                vec3(transMat * vec4(vertices[0], 1.0f)),
                vec3(transMat * vec4(vertices[1], 1.0f)),
                vec3(transMat * vec4(vertices[2], 1.0f)),
                vec3(transMat * vec4(vertices[3], 1.0f))
            };

            const int32_t indices[] = {
                0, 1, 2, 0, 2, 3
            };

            for (int32_t i = 0; i < 6; i += 3) {
                vec3 v0 = worldVertices[indices[i + 0]];
                vec3 v1 = worldVertices[indices[i + 1]];
                vec3 v2 = worldVertices[indices[i + 2]];

                vec3 normal = glm::normalize(glm::cross(v2 - v0, v1 - v0));

                vec3 intersection;
                if (GetIntersectionPos(_editorCamera.GetPosition(), rayDirection, 1000.0f, normal, v0, intersection)) {
                    if (IsInside(v0, v1, v2, intersection)) {
                        float distsqrt = glm::length2(intersection - screenToWorld);
                        if (distsqrt < minDistance) {
                            minDistance = distsqrt;
                            candidateEnttId = (uint32_t)entity;
                        }
                        break;
                    }
                }
            }
        }

        return candidateEnttId;
    }
    
    void ViewportEditor::DrawDebugComponent() {
        if (!_useEditorCamera) {
            return;
        }

        TransformComponent& transComp = _selectedEntt.GetComponent<TransformComponent>();

        if (_selectedEntt.HasComponent<BoxCollider2DComponent>()) {
            BoxCollider2DComponent& boxColliderComp = _selectedEntt.GetComponent<BoxCollider2DComponent>();

            Renderer2D::DrawLineRect(
                (uint32_t)_selectedEntt,
                transComp.worldTransform * ModelMatrix(vec3(boxColliderComp.offset, 0.0), vec3(0.0), vec3(boxColliderComp.size * 2.0f, 1.0)),
                vec4(0.0, 1.0, 0.0, 1.0)
            );
        }

        if (_selectedEntt.HasComponent<CircleCollider2DComponent>()) {
            CircleCollider2DComponent& circleColliderComp = _selectedEntt.GetComponent<CircleCollider2DComponent>();

            const vec3 toCamera = _editorCamera.GetPosition() - transComp.position;
            const float offsetZ = dot(transComp.GetWorldFront(), toCamera) < 0 ? -0.001f : 0.001f;

            Renderer2D::DrawCircle(
                (uint32_t)_selectedEntt,
                transComp.worldTransform * ModelMatrix(vec3(circleColliderComp.offset, offsetZ), vec3(0.0), vec3(circleColliderComp.radius * 2.0f, circleColliderComp.radius * 2.0f, 1.0)),
                vec4(0.0, 1.0, 0.0, 1.0),
                0.02f
            );
        }

        if (_selectedEntt.HasComponent<PointLightComponent>()) {
            PointLightComponent& pointLightComp = _selectedEntt.GetComponent<PointLightComponent>();
            DebugRender::DrawSphere(transComp.worldTransform, pointLightComp.range, vec3(0.0, 1.0, 0.0));
        }

        if (_selectedEntt.HasComponent<SpotLightComponent>()) {
            SpotLightComponent& spotLightComp = _selectedEntt.GetComponent<SpotLightComponent>();
            DebugRender::DrawCone(transComp.worldTransform, spotLightComp.range, spotLightComp.outer, spotLightComp.inner, vec3(0.0, 1.0, 0.0));
        }

        if (_selectedEntt.HasComponent<DecalComponent>()) {
            DebugRender::DrawCube(transComp.worldTransform, vec3(0.0, 1.0, 0.0));
        }

		Renderer2D::End();
    }

    void ViewportEditor::DrawOulineOfSelectedEntity(const mat4& view, const mat4& proj) {
        if (!_selectedEntt.HasComponent<MeshFilterComponent>() || !_selectedEntt.HasComponent<MeshRendererComponent>()) {
            return;
        }

		AssetHandle meshHandle = _selectedEntt.GetComponent<MeshFilterComponent>().mesh;
		auto meshAsset = AssetManager::GetAsset<MeshAsset>(meshHandle);
		if (!meshAsset) {
			return;
		}

		auto vertexBuffer = meshAsset->GetVertexBuffer();
		auto indexBuffer = meshAsset->GetIndexBuffer();
		
		auto& cmdQueue = _graphicsContext.GetCommandQueue();

		MVPMatrices mvp;
		mvp.world = _selectedEntt.GetComponent<TransformComponent>().worldTransform;
		mvp.view = view;
		mvp.projection = proj;
		_mvpConstantBuffer->Update(&mvp, sizeof(MVPMatrices));

        cmdQueue.Begin();
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		cmdQueue.SetPipeline(_outlineGraphicsPipeline);
		cmdQueue.SetVertexBuffer(meshAsset->GetVertexBuffer());
		cmdQueue.SetConstantBuffer(_mvpConstantBuffer, 0);
		cmdQueue.DrawIndexed(indexBuffer, indexBuffer->IndexCount());
        cmdQueue.End();

        cmdQueue.Execute();
    }
}