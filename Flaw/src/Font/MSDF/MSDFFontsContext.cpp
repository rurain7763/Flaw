#include "pch.h"
#include "MSDFFontsContext.h"
#include "Log/Log.h"
#include "MSDFFont.h"

namespace flaw {
	MSDFFontsContext::MSDFFontsContext() {
		_ftHandle = msdfgen::initializeFreetype();
		if (!_ftHandle) {
			throw std::runtime_error("Failed to initialize FreeType");
		}
	}

	MSDFFontsContext::~MSDFFontsContext() {
		msdfgen::deinitializeFreetype(_ftHandle);
	}

	Ref<Font> MSDFFontsContext::CreateFont(const char* filePath) {
		return CreateRef<MSDFFont>(_ftHandle, filePath);
	}

	Ref<Font> MSDFFontsContext::CreateFont(const int8_t* data, uint64_t size) {
		return CreateRef<MSDFFont>(_ftHandle, data, size);
	}
}