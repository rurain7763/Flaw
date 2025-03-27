#pragma once

#include "Core.h"

#include <vector>

namespace flaw {
	class FileSystem {
	public:
		static bool ReadFile(const char* path, std::vector<char>& out);
	};
}