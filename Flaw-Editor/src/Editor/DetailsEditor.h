#pragma once

#include <Flaw.h>

#include "EditorHelper.h"

namespace flaw {
	class DetailsEditor {
	public:
		DetailsEditor(Application& app);
		~DetailsEditor();

		void OnRender();

	private:
		Application& _app;

		Entity _selectedEntt;
	};
}