#pragma once

#include "ft2_debug.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"
#include "../base/memory_mapped_file.hpp"

namespace yg
{
  struct Font
  {
    string m_name;

    MemoryMappedFile m_fontData;

    /// information about symbol ranges
    /// ...
    /// constructor
    Font(char const * name);

    FT_Error CreateFaceID(FT_Library library, FT_Face * face);
  };

  struct GlyphCacheImpl
  {
    FT_Library m_lib;
    FT_Stroker m_stroker;

    FTC_Manager m_manager;

    FTC_ImageCache m_glyphMetricsCache;
    FTC_ImageCache m_strokedGlyphCache;
    FTC_ImageCache m_normalGlyphCache;

    FTC_CMapCache m_charMapCache;

    typedef vector<shared_ptr<Font> > TFonts;
    TFonts m_fonts;

    static FT_Error RequestFace(FTC_FaceID faceID, FT_Library library, FT_Pointer requestData, FT_Face * face);

    GlyphCacheImpl(size_t maxSize);
    ~GlyphCacheImpl();
  };
}
