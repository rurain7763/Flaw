#include "ViewportEditor.h"
#include "EditorEvents.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "Editor/ImGuizmo.h"
#include <limits>

namespace flaw {
    ViewportEditor::ViewportEditor(Application& app, EditorCamera& camera)
        : _platformContext(app.GetPlatformContext())
        , _graphicsContext(app.GetGraphicsContext())
        , _eventDispatcher(app.GetEventDispatcher())
        , _camera(camera)
    {
        CreateRequiredTextures();

        _eventDispatcher.Register<OnSelectEntityEvent>([this](const OnSelectEntityEvent& evn) {
            _selectedEntt = evn.entity;
        }, PID(this));

        _eventDispatcher.Register<WindowResizeEvent>([this](const WindowResizeEvent& evn) { CreateRequiredTextures(); }, PID(this));
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
        vec2 mousePos = vec2(Input::GetMouseX(), Input::GetMouseY());

        _graphicsContext.CaptureRenderTargetTex(_captureRenderTargetTexture);
        auto dxTexture = std::static_pointer_cast<DXTexture2D>(_captureRenderTargetTexture);

#if false
        // MEMO: 마우스 피킹 ray 테스트
        if (ImGui::IsWindowHovered() && !ImGuizmo::IsOver() && Input::GetMouseButtonDown(MouseButton::Left)) {
            MousePickingWithRay(mousePos, relativePos, vec2(currentSize.x, currentSize.y));
        }
#else
        // MEMO: 마우스 피킹 render target 테스트
        vec2 remap = Remap(
            vec2(0.0),
            vec2(currentSize.x, currentSize.y),
            mousePos - relativePos,
            vec2(0.0),
            vec2(dxTexture->GetWidth(), dxTexture->GetHeight())
        );

        if (ImGui::IsWindowHovered() && !ImGuizmo::IsOver() && Input::GetMouseButtonDown(MouseButton::Left)) {
            uint32_t id = MousePicking(remap.x, remap.y);
            for (auto&& [entity] : _scene->GetRegistry().view<entt::entity>().each()) {
                if ((uint32_t)entity == id) {
                    _selectedEntt = Entity(entity, _scene.get());
                    _eventDispatcher.Dispatch<OnSelectEntityEvent>(_selectedEntt);
                    break;
                }
            }
        }
#endif

        ImGui::Image((ImTextureID)dxTexture->GetShaderResourceView().Get(), currentSize);

		// 에디터 카메라 및 모든 scene 내의 카메라 aspect ratio 업데이트
		const float aspectRatio = currentSize.x / currentSize.y;
        _camera.SetAspectRatio(aspectRatio);
		for (auto&& [entity, cameraComp] : _scene->GetRegistry().view<CameraComponent>().each()) {
			cameraComp.aspectRatio = aspectRatio;
		}

        // MEMO: 기즈모 드로우 테스트
        if (_selectedEntt) {
            ImGuizmo::SetOrthographic(!_camera.IsPerspective());
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(currentPos.x, currentPos.y, currentSize.x, currentSize.y);

            mat4 viewMatrix = _camera.GetViewMatrix();
            mat4 projectionMatrix = _camera.GetProjectionMatrix();

            auto& enttTransComp = _selectedEntt.GetComponent<TransformComponent>();
            mat4 enttTransform = enttTransComp.GetTransform();

            static ImGuizmo::OPERATION op = ImGuizmo::OPERATION::TRANSLATE;

            bool snapping = Input::GetKey(KeyCode::LCtrl);
            float snapValue = 0.5f;
            if (snapping && op == ImGuizmo::OPERATION::ROTATE) {
                snapValue = 45.0f;
            }

            if (ImGui::IsWindowFocused()) {
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
                ImGuizmo::MODE::WORLD,
                glm::value_ptr(enttTransform),
                nullptr,
                snapping ? glm::value_ptr(glm::vec3(snapValue, snapValue, snapValue)) : nullptr
            );

            if (ImGuizmo::IsUsing()) {
                ExtractModelMatrix(enttTransform, enttTransComp.position, enttTransComp.rotation, enttTransComp.scale);
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
	}

    void ViewportEditor::CreateRequiredTextures() {
        Texture::Descriptor desc = {};
        desc.type = Texture::Type::Texture2D;

		int32_t frameBufferWidth, frameBufferHeight;
		_platformContext.GetFrameBufferSize(frameBufferWidth, frameBufferHeight);
        desc.width = frameBufferWidth;
        desc.height = frameBufferHeight;

        desc.format = PixelFormat::RGBA8;
        desc.usage = UsageFlag::Static;
        desc.bindFlags = BindFlag::ShaderResource;

        _captureRenderTargetTexture = _graphicsContext.CreateTexture2D(desc);

        desc.format = PixelFormat::R32_UINT;
        desc.usage = UsageFlag::Static;
        desc.bindFlags = BindFlag::RenderTarget;

        _idRenderTexture = _graphicsContext.CreateTexture2D(desc);

        float clearColor[4] = { std::numeric_limits<int32_t>().max(), 0.0f, 0.0f, 0.0f };
        _graphicsContext.SetRenderTexture(1, _idRenderTexture, clearColor);
    }

    uint32_t ViewportEditor::MousePicking(int32_t x, int32_t y) {
		if (x < 0 || x >= _idRenderTexture->GetWidth() || y < 0 || y >= _idRenderTexture->GetHeight()) {
            return std::numeric_limits<int32_t>().max();
		}
        Texture::Descriptor desc = {};
		desc.type = Texture::Type::Texture2D;
		desc.width = 1;
		desc.height = 1;
		desc.format = PixelFormat::R32_UINT;
		desc.usage = UsageFlag::Staging;
		desc.access = AccessFlag::Read;

		Ref<Texture> stagingTexture = _graphicsContext.CreateTexture2D(desc);

		_idRenderTexture->CopyToSub(stagingTexture, x, y, 1, 1);

        uint32_t pixel;
		stagingTexture->Fetch(&pixel, sizeof(uint32_t));

		return pixel;
	}

    uint32_t ViewportEditor::MousePickingWithRay(const vec2& mousePos, const vec2& relativePos, const vec2& currentSize) {
        vec3 screenToWorld = ScreenToWorld(
            mousePos,
            vec4(relativePos.x, relativePos.y, currentSize.x, currentSize.y),
            _camera.GetProjectionMatrix(),
            _camera.GetViewMatrix()
        );

        vec3 rayDirection = glm::normalize(screenToWorld - _camera.GetPosition());

        const std::vector<vec3> vertices = {
            { -0.5f, 0.5f, 0.0f },
            { 0.5f, 0.5f, 0.0f },
            { 0.5f, -0.5f, 0.0f },
            { -0.5f, -0.5f, 0.0f }
        };

		uint32_t candidateEnttId = std::numeric_limits<int32_t>().max();
        float minDistance = std::numeric_limits<float>().max();
        for (auto&& [entity, enttComp, transform, sprite] : _scene->GetRegistry().view<EntityComponent, TransformComponent, SpriteRendererComponent>().each()) {
            const mat4 transMat = transform.GetTransform();

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
                if (GetIntersectionPos(_camera.GetPosition(), rayDirection, 1000.0f, normal, v0, intersection)) {
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
}