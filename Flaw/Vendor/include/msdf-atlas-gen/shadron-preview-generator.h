
#pragma once

#include <msdfgen/msdfgen.h>
#include <msdfgen/msdfgen-ext.h>
#include "types.h"
#include "FontGeometry.h"

namespace msdf_atlas {

/// Generates a Shadron script that displays a string using the generated atlas
bool generateShadronPreview(const FontGeometry *fonts, int fontCount, ImageType atlasType, int atlasWidth, int atlasHeight, msdfgen::Range pxRange, const unicode_t *text, const char *imageFilename, bool fullRange, const char *outputFilename);

}
