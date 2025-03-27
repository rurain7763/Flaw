#pragma once

#include "Core.h"
#include "Shader.h"
#include "Texture.h"
#include "GraphicsData.h"
#include "GraphicsType.h"
#include "EngineTypes.h"

namespace flaw {
	class Material {
	public:
		struct StructuredData {
			int32_t elmSize;
			int32_t count;
			std::vector<uint8_t> data;
		};

		template<typename T>
		FINLINE void SetConstData(int32_t idx, const T& value) {
			static_assert(sizeof(T) == 0, "Type not supported");
		}

		template<>
		FINLINE void SetConstData<int32_t>(int32_t idx, const int32_t& value) {
			assert(idx >= 0 && idx < 4, "Index out of range");
			_userData.iAttr[idx] = value;
		}

		template<>
		FINLINE void SetConstData<float>(int32_t idx, const float& value) {
			assert(idx >= 0 && idx < 4, "Index out of range");
			_userData.fAttr[idx] = value;
		}

		template<>
		FINLINE void SetConstData<Vec2>(int32_t idx, const Vec2& value) {
			assert(idx >= 0 && idx < 4, "Index out of range");
			_userData.v2Attr[idx] = value;
		}

		template<>
		FINLINE void SetConstData<vec4>(int32_t idx, const vec4& value) {
			assert(idx >= 0 && idx < 4, "Index out of range");
			_userData.v4Attr[idx] = value;
		}

		template<>
		FINLINE void SetConstData<mat4>(int32_t idx, const mat4& value) {
			assert(idx >= 0 && idx < 4, "Index out of range");
			_userData.mAttr[idx] = value;
		}

		FINLINE void SetStructuredData(const uint32_t slot, const void* data, int32_t elmSize, int32_t count) {
			assert(elmSize % 16 == 0, "Element size must be multiple of 16");

			StructuredData sbd;
			sbd.elmSize = elmSize;
			sbd.count = count;
			sbd.data.resize(elmSize * count);
			memcpy(sbd.data.data(), data, elmSize * count);

			_structuredDatas[slot] = std::move(sbd);
		}

		FINLINE void BindShader() {
			if (_shader) {
				_shader->Bind();
			}
		}

		FINLINE void BindTextures() {
			for (const auto& [slot, texture] : _textures) {
				texture->BindToGraphicsShader(slot);
			}
		}

		FINLINE void UnbindTextures() {
			for (const auto& [_, texture] : _textures) {
				texture->Unbind();
			}
		}

		FINLINE void SetRenderDomain(const RenderDomain renderDomain) { _renderDomain = renderDomain; }
		FINLINE void SetDepthTest(const DepthTest depthTest) { _depthTest = depthTest; }
		FINLINE void SetDepthWrite(const bool depthWrite) { _depthWrite = depthWrite; }
		FINLINE void SetBlendMode(const BlendMode blendMode) { _blendMode = blendMode; }
		FINLINE void SetAlphaToCoverage(const bool alphaToCoverage) { _alphaToCoverage = alphaToCoverage; }

		FINLINE void SetShader(const Ref<GraphicsShader>& shader) { _shader = shader; }
		FINLINE void SetTexture(const TextureSlot slot, const Ref<Texture>& texture) { 
			_textures[slot] = texture;

			// TODO : this should change to be sampler state number
			_textureBindingData.slots[ENUM_INT(slot)] = 0;
		}

		FINLINE void ResetTexture(const TextureSlot slot) { 
			_textures.erase(slot); 

			_textureBindingData.slots[ENUM_INT(slot)] = -1;
		}

		FINLINE Ref<Material> Clone() const {
			Ref<Material> material = new Material();

			material->_renderDomain = _renderDomain;
			material->_depthTest = _depthTest;
			material->_depthWrite = _depthWrite;
			material->_blendMode = _blendMode;
			material->_alphaToCoverage = _alphaToCoverage;
			material->_shader = _shader;
			material->_textureBindingData = _textureBindingData;
			material->_textures = _textures;
			material->_userData = _userData;
			material->_structuredDatas = _structuredDatas;

			return material;
		}

		FINLINE const TextureBindingData& GetTextureBindingData() const { return _textureBindingData; }
		FINLINE const ConstUserData& GetConstData() const { return _userData; }
		FINLINE const std::unordered_map<uint32_t, StructuredData>& GetStructuredDatas() const { return _structuredDatas; }

		FINLINE RenderDomain GetRenderDomain() const { return _renderDomain; }
		FINLINE DepthTest GetDepthTest() const { return _depthTest; }
		FINLINE bool GetDepthWrite() const { return _depthWrite; }
		FINLINE BlendMode GetBlendMode() const { return _blendMode; }
		FINLINE bool GetAlphaToCoverage() const { return _alphaToCoverage; }

	private:
		RenderDomain _renderDomain = RenderDomain::Opaque;
		DepthTest _depthTest = DepthTest::Less;
		bool _depthWrite = true;
		BlendMode _blendMode = BlendMode::Default;
		bool _alphaToCoverage = false;

		Ref<GraphicsShader> _shader;

		TextureBindingData _textureBindingData;
		std::unordered_map<TextureSlot, Ref<Texture>> _textures;

		ConstUserData _userData;
		std::unordered_map<uint32_t, StructuredData> _structuredDatas;
	};
}