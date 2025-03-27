#pragma once

#include "Core.h"

#include <string>

namespace flaw {
	struct ProjectConfig {
		std::string name;
		std::string path;

		std::string startScene;
	};

	class Project {
	public:
		static void FromFile(const char* path);
		static void ToFile(const char* path);

		static const char* GetProjectFilePath();

		static ProjectConfig& GetConfig();
	};
}