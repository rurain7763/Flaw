#pragma once

#include <Flaw.h>

namespace flaw {
	struct OnSelectEntityEvent {
		Entity entity;

		OnSelectEntityEvent(Entity entity)
			: entity(entity)
		{
		}
	};
}