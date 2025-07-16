#include "pch.h"
#include "MonoScriptContext.h"
#include "Log/Log.h"
#include "Platform/FileSystem.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/object.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/reflection.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/mono-debug.h>
#include <fmt/format.h>

namespace flaw {
	MonoScriptContext::MonoScriptContext() {
		// TODO: this should be configurable from outside
		mono_set_dirs("C:/Program Files/Mono/lib/", "C:/Program Files/Mono/etc/");

#if _DEBUG
		const char* argv[] = {
			"--debugger-agent=transport=dt_socket,address=127.0.0.1:55555,server=y,suspend=n,loglevel=3,logfile=MonoDebugger.log",
			"--soft-breakpoints"
		};

		mono_jit_parse_options(2, (char**)argv);
		mono_debug_init(MONO_DEBUG_FORMAT_MONO);
#endif

		auto rootDomain = mono_jit_init("FlawJITRuntime");
		if (!rootDomain) {
			throw std::runtime_error("mono_jit_init failed");
		}

#if _DEBUG
		mono_debug_domain_create(rootDomain);
#endif
	}

	MonoScriptContext::~MonoScriptContext() {
		auto rootDomain = mono_get_root_domain();
		if (!rootDomain) {
			return;
		}

		mono_jit_cleanup(rootDomain);

#if _DEBUG
		mono_debug_cleanup();
#endif
	}

	void MonoScriptContext::RegisterInternalCall(const char* name, void* func) {
		mono_add_internal_call(name, func);
	}

	Ref<MonoScriptDomain> MonoScriptContext::CreateDomain() {
		return CreateRef<MonoScriptDomain>();
	}

	void MonoScriptContext::SetCurrentDomain(Ref<MonoScriptDomain> domain) {
		mono_domain_set(domain->GetMonoDomain(), true);
	}

	void MonoScriptContext::CollectGarbage() {
		//mono_gc_collect(mono_gc_max_generation());
		mono_gc_collect(0);
	}

	void MonoScriptContext::GetHeapInfo(int64_t& heapSize, int64_t& heapUsed) const {
		heapSize = mono_gc_get_heap_size();
		heapUsed = mono_gc_get_used_size();
	}

	static int GcWalkCallback(MonoObject* obj, MonoClass* klass, uintptr_t size, uintptr_t num, MonoObject** refs, uintptr_t* offsets, void* data) {
		const char* typeName = mono_type_get_name_full(mono_class_get_type(klass), MonoTypeNameFormat::MONO_TYPE_NAME_FORMAT_FULL_NAME);

		if (strstr(typeName, "Flaw") == nullptr) {
			return 0; // Skip non-Flaw objects
		}

		Log::Info("GC Object: Type: %s, Size: %d, Num: %d", typeName, size, num);
		for (uintptr_t i = 0; i < num; ++i) {
			if (refs[i]) {
				MonoClass* refClass = mono_object_get_class(refs[i]);
				const char* refTypeName = mono_type_get_name_full(mono_class_get_type(refClass), MonoTypeNameFormat::MONO_TYPE_NAME_FORMAT_FULL_NAME);
				Log::Info("  Reference %d: %s", i, refTypeName);
			}
		}

		return 0; // Continue walking
	}

	void MonoScriptContext::PrintAllGCObjects() const {
		mono_gc_walk_heap(0, GcWalkCallback, nullptr);
	}
}
