#pragma once

#include <Flaw.h>
#include "Editor/Editor.h"
#include "AssetDatabase.h"

namespace flaw {
	class MaterialEditor : public Editor {
	public:
		MaterialEditor(Application& app, const std::string& editorName, const char* assetFile);

		void OnRender() override;

	private:
		Application& _app;

		bool _dirty;
		std::string _editorName;
		Ref<MaterialAsset> _targetMaterialAsset;

		MaterialCreateSettings _editedMaterialSettings;
	};
};