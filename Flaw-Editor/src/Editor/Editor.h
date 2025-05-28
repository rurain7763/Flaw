#pragma once

#include <Flaw.h>

namespace flaw {
	class Editor {
	public:
		virtual void OnRender() = 0;

		bool IsOpen() const { return _isOpen; }
		void SetOpen(bool open) { _isOpen = open; }

	protected:
		bool _isOpen = true;
	};
}