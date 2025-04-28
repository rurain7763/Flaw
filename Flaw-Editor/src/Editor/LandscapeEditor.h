#pragma once

#include <Flaw.h>
#include "EditorCamera.h"
#include "ViewportEditor.h"
#include "ContentBrowserEditor.h"

namespace flaw {
	class LandscapeEditor {
	public:
		LandscapeEditor(
			Application& app, 
			EditorCamera& editorCam,
			ViewportEditor& viewportEditor,
			ContentBrowserEditor& contentEditor
		);

		~LandscapeEditor();

		void OnRender();

		void SetScene(const Ref<Scene>& scene);

	private:
		bool GetBrushPos(Ref<Texture2D> heightMapTex, vec2& pos);

		void UpdateBrushVBAndIB();
		void DrawBrush(const Ref<Texture2D>& heightMapTex);
		
		void UpdateLandscapeTexture(const Ref<Texture2D>& texture);

	private:
		enum class BrushType {
			Round,
			Texture,
		};

		struct LandscapeUniform {
			float deltaTime;

			uint32_t brushType;
			vec2 brushPos;

			uint32_t width;
			uint32_t height;

			uint32_t padding[2];
		};

		struct LandscapeRaycastUniform {
			vec4 rayOrigin;
			vec4 rayDirection;
			float rayLength;
			int32_t triangleCount;
			int32_t padding[2];
		};

		struct LandscapeRayHit {
			uint32_t success;
			vec3 position;
			vec3 normal;
			float distance;
		};

		struct LandscapeBrushUniform {
			mat4 modelMatrix;
			mat4 viewMatrix;
			mat4 projMatrix;

			uint32_t brushType;
			vec2 brushPos;

			uint32_t width;
			uint32_t height;

			uint32_t padding[3];
		};

		Application& _app;
		EditorCamera& _editorCamera;
		ViewportEditor& _viewportEditor;
		ContentBrowserEditor& _contentEditor;

		Ref<Scene> _scene;

		Entity _selectedEntt;

		BrushType _brushType = BrushType::Round;
		vec2 _brushPos = vec2(0.0f);
		Ref<Texture2DAsset> _brushTextureAsset;

		Ref<ComputeShader> _landscapeRaycastShader;
		Ref<ConstantBuffer> _landscapeRaycastUniformCB;
		Ref<StructuredBuffer> _landscapeRaycastTriSB;
		Ref<StructuredBuffer> _landscapeRaycastResultSB;

		Ref<GraphicsShader> _landscapeBrushShader;
		Ref<ConstantBuffer> _landscapeBrushUniformCB;

		Ref<ComputeShader> _landscapeShader;
		Ref<ConstantBuffer> _landscapeUniformCB;
		Ref<VertexBuffer> _landscapeBrushVB;
		Ref<IndexBuffer> _landscapeBrushIB;
	};
}