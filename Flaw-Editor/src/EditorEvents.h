#pragma once

#include <Flaw.h>

#include "EditorTypes.h"

namespace flaw {
	struct OnSelectEntityEvent {
		Entity entity;

		OnSelectEntityEvent(Entity entity)
			: entity(entity)
		{
		}
	};

	struct OnSceneStateChangeEvent {
		SceneState state;

		OnSceneStateChangeEvent(SceneState newState)
			: state(newState)
		{
		}
	};

	struct OnScenePauseEvent {
		bool pause;
		OnScenePauseEvent(bool pause)
			: pause(pause)
		{
		}
	};
}