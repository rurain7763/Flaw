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
		Static,  // �⺻ �뵵. GPU�� �б�� ���⸦ ����ϸ� CPU ������ �ź��Ѵ�
		Dynamic, // CPU�� ���Ⱑ �����ϴ�. GPU�� �б⸸ �����ϴ�
		Staging, // GPU���� �ڷḦ ���, �����ϰ� �װ��� �����ϰų� �߰� ���縦 ���� CPU�� �о� �鿩�� �ϴ� ���
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


