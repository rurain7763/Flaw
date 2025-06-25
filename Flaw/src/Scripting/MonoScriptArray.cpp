#include "pch.h"
#include "MonoScriptArray.h"
#include "MonoScriptObject.h"
#include "MonoScriptClass.h"

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
	MonoScriptArrayView::MonoScriptArrayView(MonoScriptClass& elementClass, MonoArray* array)
		: _domain(elementClass.GetMonoDomain())
		, _elementClass(elementClass.GetMonoClass())
		, _array(array)
	{
	}

	MonoScriptClass MonoScriptArrayView::GetElementClass() const {
		return MonoScriptClass(_domain, _elementClass);
	}

	MonoArray* MonoScriptArrayView::GetMonoArray() const {
		return _array;
	}

	MonoScriptArray::MonoScriptArray()
		: _domain(nullptr)
		, _elementClass(nullptr)
		, _gcHandle(0)
	{
	}

	MonoScriptArray::MonoScriptArray(MonoScriptClass& elementClass)
		: _domain(elementClass.GetMonoDomain())
		, _elementClass(elementClass.GetMonoClass())
		, _gcHandle(0)
	{
	}

	MonoScriptArray::MonoScriptArray(const MonoScriptObject* objects, size_t size)
		: _domain(nullptr)
		, _elementClass(nullptr)
		, _gcHandle(0)
	{
		if (size == 0) {
			throw std::invalid_argument("Size must be greater than zero");
		}

		_domain = objects[0].GetMonoDomain();
		_elementClass = objects[0].GetClass().GetMonoClass();

		Instantiate(size);
		for (int32_t i = 0; i < size; ++i) {
			SetAt(i, objects[i]);
		}
	}

	MonoScriptArray::MonoScriptArray(MonoScriptArray&& other) noexcept
		: _domain(other._domain)
		, _elementClass(other._elementClass)
		, _gcHandle(other._gcHandle)
	{
		other._gcHandle = 0;
	}

	MonoScriptArray& MonoScriptArray::operator=(MonoScriptArray&& other) noexcept {
		if (this != &other) {
			GcHandleFree();

			_domain = other._domain;
			_elementClass = other._elementClass;
			_gcHandle = other._gcHandle;
			other._gcHandle = 0;
		}

		return *this;
	}

	MonoScriptArray::~MonoScriptArray() {
		GcHandleFree();
	}

	void MonoScriptArray::Instantiate(int32_t length) {
		MonoArray* array = mono_array_new(_domain, _elementClass, length);
		if (!array) {
			throw std::runtime_error("mono_array_new failed");
		}

		GcHandleFree();
		GcHandleNew(array);
	}

	void MonoScriptArray::SetAt(int32_t index, const MonoScriptObject& value) {
		MonoArray* array = GetMonoArray();

		if (index < 0 || index >= mono_array_length(array)) {
			throw std::out_of_range("Index out of bounds");
		}

		if (value.GetMonoClass() != _elementClass) {
			throw std::runtime_error("Value class does not match array element class");
		}

		mono_array_set(array, MonoObject*, index, value.GetMonoObject());
	}

	int32_t MonoScriptArray::GetLength() const {
		return mono_array_length(GetMonoArray());
	}

	MonoArray* MonoScriptArray::GetMonoArray() const {
		return (MonoArray*)mono_gchandle_get_target(_gcHandle);
	}

	MonoScriptArrayView MonoScriptArray::GetView() const {
		return MonoScriptArrayView(GetElementClass(), GetMonoArray());
	}

	MonoScriptClass MonoScriptArray::GetElementClass() const {
		return MonoScriptClass(_domain, _elementClass);
	}

	MonoScriptArray::operator bool() const {
		return _gcHandle != 0;
	}

	void MonoScriptArray::GcHandleNew(MonoArray* array) {
		_gcHandle = mono_gchandle_new((MonoObject*)array, false);
	}

	void MonoScriptArray::GcHandleFree() {
		if (_gcHandle != 0) {
			mono_gchandle_free(_gcHandle);
			_gcHandle = 0;
		}
	}
}