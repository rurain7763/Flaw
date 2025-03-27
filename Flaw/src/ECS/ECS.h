#pragma once

#include <entt/entt.hpp>

// pointer stability
namespace entt {
	struct StableComponent {};

#if ENTT_VERSION_MAJOR == 3 && ENTT_VERSION_MINOR >= 11
    // 3.11 버전 이상
    template<typename T>
    struct storage_traits {
		static constexpr auto in_place_delete = std::is_base_of_v<StableComponent, T>;
    };
#else
	// 3.11 버전 이하
    template<typename T>
    struct component_traits {
        static constexpr auto in_place_delete = std::is_base_of_v<StableComponent, T>;
    };
#endif
}
