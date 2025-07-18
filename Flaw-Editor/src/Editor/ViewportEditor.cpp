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
		, _selectionEnabled(true)
    {
		app.SetUserViewportFunc([this](float& x, float& y, float& width, float& height) { 
			x = _viewport.x;
			y = _viewport.y;
			width = _viewport.z;
			height = _viewport.w;
        });

        CreateRequiredTextures();

        _eventDispatcher.Register<OnSelectEntityEvent>([this](const OnSelectEntityEvent& evn) { 
			if (&evn.entity.GetScene() != _scene.get()) {
				return;
			}
            _selectedEntt = evn.entity; 
        }, PID(this));

        _eventDispatcher.Register<WindowResizeEvent>([this](const WindowResizeEvent& evn) { CreateRequiredTextures(); }, PID(this));
		_eventDispatcher.Register<OnSceneStateChangeEvent>([this](const OnSceneStateChangeEvent& evn) { _useEditorCamera = evn.state == SceneState::Edit; }, PID(this));
		_eventDispatcher.Register<OnScenePauseEvent>([this](const OnScenePauseEvent& evn) { _useEditorCamera = evn.pause; }, PID(this));
		_eventDispatcher.Register<OnEditorModeChangeEvent>([this](const OnEditorModeChangeEvent& evn) { _selectionEnabled = evn.mode == EditorMode::Selection; }, PID(this));
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

                if (cameraComp.depth != 0) {
                    continue;
                }

                viewMatrix = ViewMatrix(transComp.position, transComp.rotation);
				projectionMatrix = cameraComp.GetProjectionMatrix();
				isPerspective = cameraComp.perspective;
            }
        }

        if (_selectedEntt) {
            DrawDebugComponent();
        }

        Renderer2D::End();

		auto renderTargetTex = std::static_pointer_cast<Texture2D>(Graphics::GetMainRenderPass()->GetRenderTargetTex(0));
        renderTargetTex->CopyTo(_captureRenderTargetTexture);

        auto dxTexture = std::static_pointer_cast<DXTexture2D>(_captureRenderTargetTexture);

        if (_selectionEnabled) {
		    vec2 mousePos = vec2(Input::GetMouseX(), Input::GetMouseY());

            vec2 remap = Remap(
                vec2(0.0),
                vec2(currentSize.x, currentSize.y),
                mousePos - relativePos,
                vec2(0.0),
                vec2(dxTexture->GetWidth(), dxTexture->GetHeight())
            );

            if (ImGui::IsWindowHovered() && !ImGuizmo::IsOver() && Input::GetMouseButtonDown(MouseButton::Left)) {
                entt::entity id = (entt::entity)MousePicking(remap.x, remap.y);

			    _selectedEntt = Entity();
                for (auto&& [entity] : _scene->GetRegistry().view<entt::entity>().each()) {
                    if (entity == id) {
                        _selectedEntt = Entity(entity, _scene.get());
                        break;
                    }
                }

			    _eventDispatcher.Dispatch<OnSelectEntityEvent>(_selectedEntt);
            }
        }
        
        ImGui::Image((ImTextureID)dxTexture->GetShaderResourceView(), currentSize);

        // NOTE: 기즈모 드로우
        if (_selectionEnabled && _selectedEntt) {
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
		auto idRenderTargetTex = std::static_pointer_cast<Texture2D>(mainMrt->GetRenderTargetTex(1));

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
    
    void ViewportEditor::DrawDebugComponent() {
        if (!_useEditorCamera) {
            return;
        }

        TransformComponent& transComp = _selectedEntt.GetComponent<TransformComponent>();

        if (_selectedEntt.HasComponent<BoxCollider2DComponent>()) {
            BoxCollider2DComponent& boxColliderComp = _selectedEntt.GetComponent<BoxCollider2DComponent>();

            Renderer2D::DrawLineRect(
                _selectedEntt,
                transComp.worldTransform * ModelMatrix(vec3(boxColliderComp.offset, 0.0), vec3(0.0), vec3(boxColliderComp.size * 2.0f, 1.0)),
                vec4(0.0, 1.0, 0.0, 1.0)
            );
        }

        if (_selectedEntt.HasComponent<CircleCollider2DComponent>()) {
            CircleCollider2DComponent& circleColliderComp = _selectedEntt.GetComponent<CircleCollider2DComponent>();

            const vec3 toCamera = _editorCamera.GetPosition() - transComp.position;
            const float offsetZ = dot(transComp.GetWorldFront(), toCamera) < 0 ? -0.001f : 0.001f;

            Renderer2D::DrawCircle(
                _selectedEntt,
                transComp.worldTransform * ModelMatrix(vec3(circleColliderComp.offset, offsetZ), vec3(0.0), vec3(circleColliderComp.radius * 2.0f, circleColliderComp.radius * 2.0f, 1.0)),
                vec4(0.0, 1.0, 0.0, 1.0),
                0.02f
            );
        }

		if (_selectedEntt.HasComponent<BoxColliderComponent>()) {
			BoxColliderComponent& boxColliderComp = _selectedEntt.GetComponent<BoxColliderComponent>();
			DebugRender::DrawCube(transComp.worldTransform * ModelMatrix(boxColliderComp.offset, vec3(0.0f), boxColliderComp.size), vec3(0.0, 1.0, 0.0));
		}

		if (_selectedEntt.HasComponent<SphereColliderComponent>()) {
			SphereColliderComponent& sphereColliderComp = _selectedEntt.GetComponent<SphereColliderComponent>();
			DebugRender::DrawSphere(transComp.worldTransform * ModelMatrix(sphereColliderComp.offset, vec3(0.0f), vec3(1.0f)), sphereColliderComp.radius + 0.01f, vec3(0.0, 1.0, 0.0));
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

        if (_selectedEntt.HasComponent<CameraComponent>()) {
            CameraComponent& cameraComp = _selectedEntt.GetComponent<CameraComponent>();
            if (cameraComp.perspective) {
				Frustum frustum;
				CreateFrustum(GetFovX(cameraComp.fov, cameraComp.aspectRatio), cameraComp.fov, cameraComp.nearClip, cameraComp.farClip, frustum);

				DebugRender::DrawFrustum(frustum, transComp.worldTransform, vec3(0.0, 1.0, 0.0));
            }
        }

        if (_selectedEntt.HasComponent<CanvasComponent>()) {
			auto& rectLayoutComp = _selectedEntt.GetComponent<RectLayoutComponent>();
            Renderer2D::DrawLineRect(
                _selectedEntt,
                transComp.worldTransform * ModelMatrix(vec3(0.0), vec3(0.0), vec3(rectLayoutComp.sizeDelta, 0.0)),
                vec4(1.0, 1.0, 1.0, 1.0)
            );
        }

		Renderer2D::End();
    }
}