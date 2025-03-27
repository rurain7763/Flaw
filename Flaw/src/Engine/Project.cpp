#include "pch.h"
#include "Project.h"
#include "Serialization.h"
#include "Log/Log.h"

namespace flaw {
	static const char* s_projectFilePath;
	static ProjectConfig s_config;

	void Project::FromFile(const char* path) {
		YAML::Node node = YAML::LoadFile(path);
		if (!node) {
			Log::Error("Failed to load file %s", path);
			return;
		}

		auto root = node["Project"];
		if (root) {
			Deserialize(root, s_config);
		}

		s_projectFilePath = path;
	}

	void Project::ToFile(const char* path) {
		std::ofstream file(path);
		YAML::Emitter out;

		out << YAML::BeginMap;
		{
			out << YAML::Key << "Project" << YAML::Value;
			Serialize(out, s_config);
		}
		out << YAML::EndMap;

		file << out.c_str();
		file.close();
	}

	const char* Project::GetProjectFilePath() {
		return s_projectFilePath;
	}

	ProjectConfig& Project::GetConfig() {
		return s_config;
	}
}