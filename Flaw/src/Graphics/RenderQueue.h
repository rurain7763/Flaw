#pragma once

#include "Core.h"

#include "Mesh.h"
#include "Material.h"
#include "GraphicsType.h"

namespace flaw {
	class RenderQueue {
	public:
		struct RenderItem {
			RenderDomain domain;
			Ref<Mesh> mesh;
			Ref<Material> material;
			std::function<void()> renderFunc;
		};

		FINLINE void Open() {
			assert(!_isOpen, "RenderQueue is already open");
			_isOpen = true;
			_domainIndex = ENUM_INT(RenderDomain::Count);
			_itemIndex = 0;
			_itemCount = 0;

			for (uint32_t i = 0; i < ENUM_INT(RenderDomain::Count); i++) {
				_items[i].clear();
			}
		}

		FINLINE void Close() {
			// TODO: sort in domain order or grouping by material

			_isOpen = false;
		}

		FINLINE void Push(const Ref<Mesh>& mesh, const Ref<Material>& material, const std::function<void()>& renderFunc) {
			assert(_isOpen, "RenderQueue is not open");
			const RenderDomain domain = material->GetRenderDomain();
			_items[ENUM_INT(domain)].push_back({ domain, mesh, material, renderFunc });
			_domainIndex = std::min(_domainIndex, ENUM_INT(domain));
			_itemCount++;
		}

		FINLINE const RenderItem& Pop() {
			assert(!_isOpen, "RenderQueue is not closed");

			while (_domainIndex < ENUM_INT(RenderDomain::Count)) {
				if (_itemIndex < _items[_domainIndex].size()) {
					_itemCount--;
					return _items[_domainIndex][_itemIndex++];
				}
				_domainIndex++;
				_itemIndex = 0;
			}

			throw std::out_of_range("RenderQueue is empty");
		}

		FINLINE bool Empty() const {
			return _itemCount <= 0;
		}

	private:
		std::vector<RenderItem> _items[ENUM_INT(RenderDomain::Count)];
		bool _isOpen = false;
		int32_t _domainIndex = 0;
		int32_t _itemIndex = 0;
		int32_t _itemCount = 0;
	};
}
