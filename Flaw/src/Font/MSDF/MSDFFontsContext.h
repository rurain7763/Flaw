#pragma once

#include "Font/FontsContext.h"

#define MSDFGEN_PUBLIC
#include <msdf-atlas-gen/msdf-atlas-gen.h>

namespace flaw {
	class MSDFFontsContext : public FontContext {
	public:
		MSDFFontsContext();
		~MSDFFontsContext();

		Ref<Font> CreateFont(const char* filePath) override;
		Ref<Font> CreateFont(const int8_t* data, uint64_t size) override;

	private:
		msdfgen::FreetypeHandle* _ftHandle;
	};
}