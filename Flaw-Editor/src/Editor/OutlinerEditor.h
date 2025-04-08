#pragma once

#include <Flaw.h>

namespace flaw {
	class OutlinerEditor {
	public:
		OutlinerEditor(Application& app);
		~OutlinerEditor();

		void OnRender();

		void SetScene(const Ref<Scene>& scene);
		const Entity& GetSelectedEntity() const { return _selectedEntt; }

	private:
		void DrawEntityNode(const Entity& entity, bool recursive);

	private:
		Application& _app;
		EventDispatcher& _eventDispatcher;

		Ref<Scene> _scene;
		Entity _selectedEntt;
	};
}