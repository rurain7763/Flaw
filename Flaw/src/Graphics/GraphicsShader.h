#pragma once

#include "Core.h"
#include "GraphicsType.h"

#include <vector>

namespace flaw {
	class GraphicsShader {
	public:
		struct InputElement {
			enum class ElementType {
				Float,
				Uint32
			};

			const char* name;
			ElementType type;
			uint32_t count;
			uint32_t offset;
			bool normalized;
		};

		GraphicsShader() = default;
		virtual ~GraphicsShader() = default;

		virtual void CreateInputLayout() = 0;

		virtual void Bind() = 0;

		template <typename T>
		void AddInputElement(const char* name, uint32_t count, bool normalized = false) {
			FASSERT(false, "Invalid type");
		}

		template <>
		void AddInputElement<float>(const char* name, uint32_t count, bool normalized) {
			InputElement attribute;
			attribute.name = name;
			attribute.type = InputElement::ElementType::Float;
			attribute.count = count;
			attribute.normalized = normalized;
			attribute.offset = _stride;
			_stride += sizeof(float) * count;
			_inputElements.push_back(std::move(attribute));
		}

		template <>
		void AddInputElement<uint32_t>(const char* name, uint32_t count, bool normalized) {
			InputElement attribute;
			attribute.name = name;
			attribute.type = InputElement::ElementType::Uint32;
			attribute.count = count;
			attribute.normalized = normalized;
			attribute.offset = _stride;
			_stride += sizeof(uint32_t) * count;
			_inputElements.push_back(std::move(attribute));
		}

	protected:
		uint32_t _stride = 0;
		std::vector<InputElement> _inputElements;
	};
}


