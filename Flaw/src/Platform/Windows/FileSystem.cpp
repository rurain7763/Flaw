#include "pch.h"
#include "Platform/FileSystem.h"

#include <fstream>

namespace flaw {
	bool FileSystem::ReadFile(const char* path, std::vector<char>& out) {
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file.is_open()) {
			return false;
		}

		auto end = file.tellg();
		file.seekg(0, std::ios::beg);
		size_t size = end - file.tellg();

		out.resize(size);
		file.read((char*)out.data(), size);
		file.close();

		return true;
	}
}