#pragma once

#include <Flaw.h>
#include "Editor/Editor.h"
#include "AssetDatabase.h"
#include "OutlinerEditor.h"
#include "DetailsEditor.h"

namespace flaw {
	class PrefabEditor : public Editor {
	public:
		PrefabEditor(Application& app, const std::string& editorName, const char* assetFile);

		void OnRender() override;

	private:
		Application& _app;

		OutlinerEditor _outlinerEditor;
		DetailsEditor _detailsEditor;

		std::string _editorName;
		Ref<PrefabAsset> _targetPrefabAsset;

		PrefabCreateSettings _editedPrefabSettings;

		Ref<Scene> _scene;
		Entity _rootEntity;
	};
};