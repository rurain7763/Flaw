#pragma once

#include <Flaw.h>
#include "EditorHelper.h"

namespace flaw {
	class ParticleComponentDrawer {
	public:
		static bool Draw(Entity& engity);

	private:
		static bool DrawEmissionModule(EmissionModule& module);
		static bool DrawShapeModule(ShapeModule& module);
		static bool DrawRandomSpeedModule(RandomSpeedModule& module);
		static bool DrawRandomColorModule(RandomColorModule& module);
		static bool DrawRandomSizeModule(RandomSizeModule& module);
		static bool DrawColorOverLifetimeModule(ColorOverLifetimeModule& module);
		static bool DrawSizeOverLifetimeModule(SizeOverLifetimeModule& module);
		static bool DrawNoiseModule(NoiseModule& module);
		static bool DrawRendererModule(RendererModule& module);
	};
}