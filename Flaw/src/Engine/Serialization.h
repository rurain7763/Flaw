#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Mesh.h"
#include "Utils/SerializationArchive.h"
#include "Utils/Raycast.h"

#include <yaml-cpp/yaml.h>

namespace flaw {
	class Entity;
	class Scene;
	struct ProjectConfig;

	void Serialize(YAML::Emitter& out, ProjectConfig& config);
	void Serialize(YAML::Emitter& out, Entity& entity);
	void Serialize(YAML::Emitter& out, Scene& scene);

	void Deserialize(const YAML::Node& node, ProjectConfig& config);
	void Deserialize(const YAML::Node& node, Entity& entity);
	void Deserialize(const YAML::Node& node, Scene& scene);

	template<>
	struct Serializer<vec2> {
		static void Serialize(SerializationArchive& archive, const vec2& value) {
			archive << value.x;
			archive << value.y;
		}

		static void Deserialize(SerializationArchive& archive, vec2& value) {
			archive >> value.x;
			archive >> value.y;
		}
	};

	template<>
	struct Serializer<vec3> {
		static void Serialize(SerializationArchive& archive, const vec3& value) {
			archive << value.x;
			archive << value.y;
			archive << value.z;
		}

		static void Deserialize(SerializationArchive& archive, vec3& value) {
			archive >> value.x;
			archive >> value.y;
			archive >> value.z;
		}
	};

	template<>
	struct Serializer<Vertex3D> {
		static void Serialize(SerializationArchive& archive, const Vertex3D& value) {
			archive << value.position;
			archive << value.texcoord;
			archive << value.tangent;
			archive << value.normal;
			archive << value.binormal;
		}

		static void Deserialize(SerializationArchive& archive, Vertex3D& value) {
			archive >> value.position;
			archive >> value.texcoord;
			archive >> value.tangent;
			archive >> value.normal;
			archive >> value.binormal;
		}
	};

	template<>
	struct Serializer<Mesh> {
		static void Serialize(SerializationArchive& archive, const Mesh& value) {
			archive << value.topology;
			archive << value.vertices;
			archive << value.indices;
			archive << value.boundingSphereCenter;
			archive << value.boundingSphereRadius;
		}

		static void Deserialize(SerializationArchive& archive, Mesh& value) {
			archive >> value.topology;
			archive >> value.vertices;
			archive >> value.indices;
			archive >> value.boundingSphereCenter;
			archive >> value.boundingSphereRadius;
		}
	};
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
}