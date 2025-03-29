#pragma once

#include <stdint.h>
#include <memory>
#include <iostream>
#include <string>

#ifdef _WIN32
#ifdef FL_DYNAMIC_LIB
	#ifdef FL_DLL_EXPORT
		#define FAPI __declspec(dllexport)
	#else
		#define FAPI __declspec(dllimport)
	#endif
#else
	#define FAPI
#endif
#else
	#error Flaw only supports Windows!
#endif

#ifdef FL_ENABLE_ASSERTS
	#define FASSERT(x, ...) { if(!(x)) { std::cerr << "Assertion Failed: " << __VA_ARGS__ << std::endl; __debugbreak(); } }
#else
	#define FASSERT(x, ...)
#endif

namespace flaw {
	inline uint64_t PID(void* ptr) noexcept {
		return reinterpret_cast<uint64_t>(ptr);
	}

	template <typename T>
	inline std::string_view TypeName() {
		std::string_view name = typeid(T).name();
		for (int32_t i = name.size() - 1; i >= 0; --i) {
			if (name[i] == ' ' || name[i] == ':') {
				return name.substr(i + 1);
			}
		}
		return name;
	}

	template <typename T>
	using Ref = std::shared_ptr<T>;

	template <typename T, typename... Args>
	constexpr Ref<T> CreateRef(Args&&... args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template <typename T>
	using Scope = std::unique_ptr<T>;

	template <typename T, typename... Args>
	constexpr Scope<T> CreateScope(Args&&... args) {
		return std::make_unique<T>(std::forward<Args>(args)...);
	}
}
