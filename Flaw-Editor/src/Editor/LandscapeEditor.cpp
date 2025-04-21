#include "LandscapeEditor.h"

namespace flaw {
	LandscapeEditor::LandscapeEditor(Application& app)
		: _app(app)
	{
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
	}

	void LandscapeEditor::OnRender() {
		
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