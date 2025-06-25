#pragma once

#include "Core.h"
#include "MonoScriptClass.h"
#include "MonoScriptClassField.h"
#include "MonoScriptDomain.h"
#include "MonoScriptObject.h"
#include "MonoScriptArray.h"

namespace flaw {
	class MonoScriptContext {
	public:
		MonoScriptContext();
		~MonoScriptContext();

		void RegisterInternalCall(const char* name, void* func);

		Ref<MonoScriptDomain> CreateDomain();

		void SetCurrentDomain(Ref<MonoScriptDomain> domain);

		void CollectGarbage();

		void GetHeapInfo(int64_t& heapSize, int64_t& heapUsed) const;

		void PrintAllGCObjects() const;
	};
}

