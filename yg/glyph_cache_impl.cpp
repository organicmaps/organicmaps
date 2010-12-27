#include "../base/SRC_FIRST.hpp"
#include "glyph_cache_impl.hpp"

#include <../cache/ftcglyph.h>
#include <../cache/ftcimage.h>
#include <../cache/ftcsbits.h>
#include <../cache/ftccback.h>
#include <../cache/ftccache.h>


namespace yg
{
  Font::Font(char const * name) : m_name(name), m_fontData(name, true)
  {
  }

  FT_Error Font::CreateFaceID(FT_Library library, FT_Face *face)
  {
    return FT_New_Memory_Face(library, (unsigned char*)m_fontData.data(), m_fontData.size(), 0, face);
  }

  GlyphCacheImpl::GlyphCacheImpl(size_t maxSize)
  {
    FTCHECK(FT_Init_FreeType(&m_lib));

    /// Initializing caches
    FTCHECK(FTC_Manager_New(m_lib, 3, 10, maxSize, &RequestFace, 0, &m_manager));

    FTCHECK(FTC_ImageCache_New(m_manager, &m_normalGlyphCache));
    FTCHECK(FTC_ImageCache_New(m_manager, &m_glyphMetricsCache));

    /// Initializing stroker
    FTCHECK(FT_Stroker_New(m_lib, &m_stroker));
    FT_Stroker_Set(m_stroker, 2 * 64, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

    FTCHECK(FTC_StrokedImageCache_New(m_manager, &m_strokedGlyphCache, m_stroker));

    FTCHECK(FTC_CMapCache_New(m_manager, &m_charMapCache));

  }

  GlyphCacheImpl::~GlyphCacheImpl()
  {
    FTC_Manager_Done(m_manager);
    FT_Stroker_Done(m_stroker);
    FT_Done_FreeType(m_lib);
  }

  FT_Error GlyphCacheImpl::RequestFace(FTC_FaceID faceID, FT_Library library, FT_Pointer requestData, FT_Face * face)
  {
    //GlyphCacheImpl * glyphCacheImpl = reinterpret_cast<GlyphCacheImpl*>(requestData);
    Font * font = reinterpret_cast<Font*>(faceID);
    return font->CreateFaceID(library, face);
  }
}
