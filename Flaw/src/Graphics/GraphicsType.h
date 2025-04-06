#pragma once

namespace flaw {
	enum class RenderDomain {
		Opaque,
		Masked,
		Transparent,
		PostProcess,
		Debug,
		Count
	};

	enum class UsageFlag {
		Static,  // 기본 용도. GPU의 읽기와 쓰기를 허용하며 CPU 접근은 거부한다
		Dynamic, // CPU의 쓰기가 가능하다. GPU는 읽기만 가능하다
		Staging, // GPU에서 자료를 계산, 조작하고 그것을 저장하거나 추가 조사를 위해 CPU로 읽어 들여야 하는 경우
	};

	enum AccessFlag {
		Write = 0x1,
		Read = 0x2
	};

	enum BindFlag {
		ShaderResource = 0x1,
		UnorderedAccess = 0x2,
		RenderTarget = 0x4,
		DepthStencil = 0x8
	};

	enum class PrimitiveTopology {
		TriangleList,
		TriangleStrip,
		LineList,
		LineStrip,
		PointList
	};

	enum class CullMode {
		None,
		Front,
		Back
	};

	enum class FillMode {
		Solid,
		Wireframe
	};

	enum class DepthTest {
		Less,
		LessEqual,
		Greater,
		GreaterEqual,
		Equal,
		NotEqual,
		Always
	};

	enum class BlendMode {
		Default,  // SRC : 1, DST : 0
		Alpha,    // SRC : SRC_ALPHA, DST : 1 - SRC_ALPHA
		Additive, // SRC : SRC_ALPHA, DST : 1
	};

	enum class PixelFormat {
		UNDEFINED,
		RGBA8,
		RGBA32F,
		RGB8,
		BGRX8,
		RG8,
		R8,
		R32_UINT,
		D24S8_UINT,
	};

	enum TextureSlot {
		_0 = 0,
		_1,
		_2,
		_3,
		_4,

		Cube0,
		Cube1,
		Cube2,
		Cube3,

		Array0,
		Array1,
		Array2,
		Array3,

		Count
	};

	// TODO: temporary solution
	enum class ReservedConstantBufferSlot {
		MVPMatrices = 0,
		ConstUserData,
		Animation2DData,
		SkyLightData,
		ShaderGlobalData,
		TextureBindingData,
		Count
	};

	// TODO: temporary solution
	enum class ReservedStructuredBufferSlot {
		PointLight = TextureSlot::Count,
		DirectionalLight,
		SpotLight,
		Count
	};

	enum ShaderCompileFlag {
		Vertex = 0x1,
		Pixel = 0x2,
		Geometry = 0x4,
		Hull = 0x8,
		Domain = 0x10
	};
}


