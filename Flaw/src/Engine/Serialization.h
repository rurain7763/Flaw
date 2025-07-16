#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Graphics.h"
#include "Assets.h"
#include "Utils/SerializationArchive.h"
#include "Utils/Raycast.h"
#include "Utils/UUID.h"
#include "Mesh.h"
#include "Prefab.h"

#include <yaml-cpp/yaml.h>
#include <unordered_map>

namespace flaw {
	class Entity;
	class Scene;
	struct ProjectConfig;

	void Serialize(YAML::Emitter& out, ProjectConfig& config);
	void Serialize(YAML::Emitter& out, Entity& entity);
	void Serialize(YAML::Emitter& out, Scene& scene);

	void Deserialize(const YAML::Node& node, ProjectConfig& config);
	void Deserialize(
		const YAML::Node& node, 
		Entity& entity, 
		const std::unordered_map<std::string, std::function<void(const YAML::iterator::value_type&, Entity&)>>& userComponentDeserializer = std::unordered_map<std::string, std::function<void(const YAML::iterator::value_type&, Entity&)>>()
	);
	void Deserialize(const YAML::Node& node, Scene& scene);
}

namespace YAML {
	Emitter& operator<<(YAML::Emitter& out, vec2 vec);
	Emitter& operator<<(YAML::Emitter& out, vec3& vec);
	Emitter& operator<<(YAML::Emitter& out, vec4& vec);

	template<>
	struct convert<vec2> {
		static Node encode(const vec2& rhs) {
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			return node;
		}

		static bool decode(const Node& node, vec2& rhs) {
			if (!node.IsSequence() || node.size() != 2) {
				return false;
			}
			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<vec3> {
		static Node encode(const vec3& rhs) {
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, vec3& rhs) {
			if (!node.IsSequence() || node.size() != 3) {
				return false;
			}
			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<vec4> {
		static Node encode(const vec4& rhs) {
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			return node;
		}

		static bool decode(const Node& node, vec4& rhs) {
			if (!node.IsSequence() || node.size() != 4) {
				return false;
			}
			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<flaw::UUID> {
		static Node encode(const flaw::UUID& uuid) {
			Node node;
			node = static_cast<uint64_t>(uuid);
			return node;
		}

		static bool decode(const Node& node, flaw::UUID& uuid) {
			if (!node.IsScalar()) {
				return false;
			}
			uuid = flaw::UUID(std::stoull(node.Scalar()));

			return true;
		}
	};
}