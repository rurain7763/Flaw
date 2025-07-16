#pragma once

#include <Flaw.h>
#include "Editor/Editor.h"
#include "AssetDatabase.h"

namespace flaw {
	class SkeletonEditor : public Editor {
	public:
		SkeletonEditor(Application& app, const std::string& editorName, const char* assetFile);

		void OnRender() override;

	private:
		void DrawBone(int32_t currentNodeIndex);
		void DrawSkeleton(Ref<Skeleton> skeleton);
		void DrawSockets(Ref<Skeleton> skeleton);

	private:
		Application& _app;

		bool _dirty;
		std::string _editorName;
		Ref<SkeletonAsset> _targetSkeletonAsset;

		SkeletonCreateSettings _editedSkeletonSettings;
	};
};