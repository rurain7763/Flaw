#pragma once

#include "Core.h"

#include <vector>

namespace flaw {
	class FileSystem {
	public:
		static bool MakeFile(const char* path, const char* data = nullptr, uint64_t size = 0);

		static bool WriteFile(const char* path, const char* data, uint64_t size);
		static bool ReadFile(const char* path, std::vector<char>& out);

		static uint64_t FileIndex(const char* path);
	};
}