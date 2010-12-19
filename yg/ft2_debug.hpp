#pragma once

#include <ft2build.h>
#include <freetype/ftgzip.h>
#include <freetype/ftcache.h>

#include "../base/logging.hpp"

#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_GLYPH_H

char const * FT_Error_Description(FT_Error error);
void CheckError(FT_Error error);

#define FTCHECK(x) do {FT_Error e = (x); CheckError(e);} while (false)
