#pragma once

#include "Core.h"
#include "MonoScriptTypes.h"

namespace flaw {
	class MonoScriptClass;
	class MonoScriptObject;

	class MonoScriptArrayView {
	public:
		MonoScriptArrayView(MonoScriptClass& elementClass, MonoArray* array);

		MonoScriptClass GetElementClass() const;
		MonoArray* GetMonoArray() const;

	private:
		MonoDomain* _domain;
		MonoClass* _elementClass;
		MonoArray* _array;
	};

	class MonoScriptArray {
	public:
		MonoScriptArray();
		MonoScriptArray(MonoScriptClass& elementClass);
		MonoScriptArray(const MonoScriptObject* objects, size_t size);
		MonoScriptArray(const MonoScriptArray&) = delete;		

		MonoScriptArray(MonoScriptArray&& other) noexcept;

		~MonoScriptArray();

		MonoScriptArray& operator=(const MonoScriptArray&) = delete;
		MonoScriptArray& operator=(MonoScriptArray&& other) noexcept;

		void Instantiate(int32_t length);

		void SetAt(int32_t index, const MonoScriptObject& value);

		int32_t GetLength() const;

		MonoArray* GetMonoArray() const;

		MonoScriptArrayView GetView() const;

		MonoScriptClass GetElementClass() const;

		explicit operator bool() const;

	private:
		void GcHandleNew(MonoArray* array);
		void GcHandleFree();

	private:
		MonoDomain* _domain;
		MonoClass* _elementClass;

		uint32_t _gcHandle;
	};
}