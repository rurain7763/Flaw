#pragma once

#include "Core.h"
#include "Graphics.h"

namespace flaw {
	class PrimitiveManager {
	public:
		static void Init();
		static void Cleanup();

		static Ref<Mesh> GetCubeMesh();
		static Ref<Mesh> GetSphereMesh();
	};
}