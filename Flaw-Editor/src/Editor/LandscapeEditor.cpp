#include "LandscapeEditor.h"
#include "DebugRender.h"

namespace flaw {
	LandscapeEditor::LandscapeEditor(Application& app, EditorCamera& editorCam, ViewportEditor& viewportEditor)
		: _app(app)
		, _editorCamera(editorCam)
		, _viewportEditor(viewportEditor)
	{
#if false
		//TODO: test code
		_landscapeUniformCB = Graphics::CreateConstantBuffer(sizeof(LandscapeUniform));

		Texture2D::Descriptor desc = {};
		desc.width = 1024;
		desc.height = 1024;
		desc.format = PixelFormat::RGBA32F;
		desc.usage = UsageFlag::Static;
		desc.bindFlags = BindFlag::ShaderResource | BindFlag::UnorderedAccess;
	
		_landscapeTexture = Graphics::CreateTexture2D(desc);

		_landscapeShader = Graphics::CreateComputeShader("Resources/Shaders/landscape_compute.fx");

		_landscapePipeline = Graphics::CreateComputePipeline();
		_landscapePipeline->SetShader(_landscapeShader);
#endif
	}

	void DrawBVH(const std::vector<BVHNode>& bvhNodes, const std::vector<BVHTriangle>& bvhTriangles, int32_t current, const mat4& transform) {
		auto& node = bvhNodes[current];

		if (node.IsLeaf()) {
			vec3 size = ExtractScale(transform) * (node.boundingBox.max - node.boundingBox.min);
			vec3 center = transform * vec4(node.boundingBox.center, 1.0);
			DebugRender::DrawCube(ModelMatrix(center, vec3(0), size), vec3(0, 1, 0));
		}
		else {
			DrawBVH(bvhNodes, bvhTriangles, node.childA, transform);
			DrawBVH(bvhNodes, bvhTriangles, node.childB, transform);
		}
	}

	void LandscapeEditor::OnRender() {
		// TODO: check if landscape mode, no play mode

		if (!_scene) {
			return;
		}

		vec2 mousePos = vec2(Input::GetMouseX(), Input::GetMouseY());

		Ray ray = {};
		ray.origin = ScreenToWorld(mousePos, _viewportEditor.GetViewport(), _editorCamera.GetProjectionMatrix(), _editorCamera.GetViewMatrix());
		ray.direction = glm::normalize(ray.origin - _editorCamera.GetPosition());
		ray.length = 1000.0f;

		auto& landscapeSys = _scene->GetLandscapeSystem();
		for (auto&& [entity, enttComp, landscapeComp, transformComp] : _scene->GetRegistry().view<EntityComponent, LandScaperComponent, TransformComponent>().each()) {
			auto& landscape = landscapeSys.GetLandscape(enttComp.uuid);
			DrawBVH(landscape.mesh->bvhNodes, landscape.mesh->bvhTriangles, 0, transformComp.worldTransform);

			mat4 invTransform = glm::inverse(transformComp.worldTransform);

			Ray localRay = {};
			localRay.origin = invTransform * vec4(ray.origin, 1.0f);
			localRay.direction = invTransform * vec4(ray.direction, 0.0f);
			localRay.length = ray.length;

			RayHit hit = {};
			if (RaycastBVH(landscape.mesh->bvhNodes, landscape.mesh->bvhTriangles, localRay, hit)) {
				hit.position = transformComp.worldTransform * vec4(hit.position, 1.0f);
				hit.normal = transformComp.worldTransform * vec4(hit.normal, 0.0f);

				DebugRender::DrawCube(ModelMatrix(hit.position, vec3(0), vec3(1.0f)), vec3(1.0f, 0.0f, 0.0f));
			}
		}
	}

	void LandscapeEditor::UpdateLandscapeTexture() {
		auto& cmdQueue = Graphics::GetCommandQueue();

		LandscapeUniform landscapeUniform = {};
		landscapeUniform.width = _landscapeTexture->GetWidth();
		landscapeUniform.height = _landscapeTexture->GetHeight();
		_landscapeUniformCB->Update(&landscapeUniform, sizeof(LandscapeUniform));

		cmdQueue.Begin();
		cmdQueue.SetComputePipeline(_landscapePipeline);
		cmdQueue.SetComputeConstantBuffer(_landscapeUniformCB, 0);
		cmdQueue.SetComputeTexture(_landscapeTexture, BindFlag::UnorderedAccess, 0);

		cmdQueue.Dispatch(
			CalculateDispatchGroupCount(32, _landscapeTexture->GetWidth()), 
			CalculateDispatchGroupCount(32, _landscapeTexture->GetHeight()), 
			1
		);

		cmdQueue.End();

		cmdQueue.Execute();
	}
}