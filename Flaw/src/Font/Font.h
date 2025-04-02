#pragma once

#include "Core.h"

namespace flaw {
	struct FontData;

	class Font {
	public:
		Font(const char* filePath);
		~Font();

		FontData& GetFontData() const { return *_data; }

	private:
		FontData* _data;
	};
}