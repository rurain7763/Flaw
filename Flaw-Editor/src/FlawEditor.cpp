#include "FlawEditor.h"

namespace flaw {
	FlawEditor::FlawEditor(const ApplicationProps& props)
		: Application(props)
	{
		_editorLayer = std::make_unique<EditorLayer>(*this);
		PushLayer(_editorLayer.get());
	}

	FlawEditor::~FlawEditor() {
		RemoveLayer(_editorLayer.get());
	}
}