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
			bool normalized;

			uint32_t offset;
		};

		GraphicsShader() = default;
		virtual ~GraphicsShader() = default;

		virtual void CreateInputLayout() = 0;

		virtual void Bind() = 0;

		void AddInputElement(InputElement& element) {
			uint32_t typeSize = 0; 
			if (element.type == InputElement::ElementType::Float) {
				typeSize = sizeof(float);
			}
			else if (element.type == InputElement::ElementType::Uint32) {
				typeSize = sizeof(uint32_t);
			}
			else {
				FASSERT(false, "Invalid type");
			}

			element.offset = _stride;
			_stride += typeSize * element.count;
			_inputElements.push_back(std::move(element));
		}

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


