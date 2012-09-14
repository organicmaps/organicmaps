#pragma once

#include <ft2build.h>
#include <freetype/ftgzip.h>
#include <freetype/ftcache.h>

#include "../base/logging.hpp"

#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_GLYPH_H

namespace ft2_impl
{
  void CheckError(FT_Error error);
}

#define FTCHECK(x) do { FT_Error e = (x); ft2_impl::CheckError(e); } while (false)
#define FTCHECKRETURN(x) do { FT_Error e = (x); if (e != 0) { ft2_impl::CheckError(e); return; } } while (false)
