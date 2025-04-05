#include "FlawEditor.h"
#include "AssetDatabase.h"

namespace flaw {
	FlawEditor::FlawEditor(const ApplicationProps& props)
		: Application(props)
	{
		AssetDatabase::Init(*this);

		_editorLayer = std::make_unique<EditorLayer>(*this);
		PushLayer(_editorLayer.get());
	}

	FlawEditor::~FlawEditor() {
		RemoveLayer(_editorLayer.get());

		AssetDatabase::Cleanup();
	}
}