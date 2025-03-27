#pragma once

#include <Flaw.h>

#include "EditorLayer.h"

namespace flaw {
	class FlawEditor : public Application
	{
	public:
		FlawEditor(const ApplicationProps& props);
		~FlawEditor();

	private:
		std::unique_ptr<EditorLayer> _editorLayer;
	};
}

