#include "pch.h"
#include "Platform/FileSystem.h"

#include <Windows.h>
#include <fstream>

namespace flaw {
	bool FileSystem::MakeFile(const char* path, const int8_t* data, uint64_t size) {
		std::ofstream file(path, std::ios::binary);
		if (!file.is_open()) {
			return false;
		}

		if (size) {
			file.write((const char*)data, size);
		}

		file.close();

		return true;
	}

	bool FileSystem::WriteFile(const char* path, const int8_t* data, uint64_t size) {
		std::ofstream file(path, std::ios::binary);
		if (!file.is_open()) {
			return false;
		}

		file.write((const char*)data, size);
		file.close();

		return true;
	}

	bool FileSystem::ReadFile(const char* path, std::vector<int8_t>& out) {
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

	uint64_t FileSystem::FileIndex(const char* path) {
		HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) return -1;

		BY_HANDLE_FILE_INFORMATION fileInfo;
		if (!GetFileInformationByHandle(hFile, &fileInfo)) {
			CloseHandle(hFile);
			return -1;
		}
		CloseHandle(hFile);

		return (static_cast<uint64_t>(fileInfo.nFileIndexHigh) << 32) | fileInfo.nFileIndexLow;
	}
}