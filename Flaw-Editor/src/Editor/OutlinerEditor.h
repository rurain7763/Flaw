#pragma once

#include <Flaw.h>
#include "Editor.h"

namespace flaw {
	class OutlinerEditor : public Editor {
	public:
		OutlinerEditor(Application& app, const std::string& name);
		~OutlinerEditor();

		void OnRender() override;

		void SetScene(const Ref<Scene>& scene);
		const Entity& GetSelectedEntity() const { return _selectedEntt; }

	private:
		void DrawEntityNode(const Entity& entity, bool recursive);

	private:
		Application& _app;
		EventDispatcher& _eventDispatcher;

		std::string _name;

		Ref<Scene> _scene;
		Entity _selectedEntt;
	};
}