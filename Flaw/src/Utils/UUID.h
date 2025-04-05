#pragma once

#include "Core.h"

#include <xhash>

namespace flaw {
	class UUID {
	public:
		UUID();
		UUID(uint64_t id);
		UUID(const UUID& other) = default;

		void Generate();

		UUID& operator=(const UUID& other) {
			if (this != &other) {
				_id = other._id;
			}
			return *this;
		}

		operator uint64_t() const { return _id; }

	private:
		friend struct std::hash<UUID>;

		uint64_t _id;
	};
}

namespace std {
	template<>
	struct hash<flaw::UUID> {
		size_t operator()(const flaw::UUID& uuid) const {
			return uuid._id;
		}
	};
}