#include "../base/SRC_FIRST.hpp"
#include "../base/logging.hpp"
#include "../coding/lodepng_io.hpp"
#include "../std/vector.hpp"
#include "../std/map.hpp"
#include <boost/gil/gil_all.hpp>
#include "glyph_cache.hpp"
#include "data_formats.hpp"
#include "internal/opengl.hpp"

#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_GLYPH_H

#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };

const struct
{
  int          err_code;
  const char*  err_msg;
} ft_errors[] =

#include FT_ERRORS_H

void CheckError(FT_Error error)
{
  if (error != 0)
  {
    int i = 0;
    while (ft_errors[i].err_code != 0)
    {
      if (ft_errors[i].err_code == error)
      {
        LOG(LINFO, ("FT_Error : ", ft_errors[i].err_msg));
        break;
      }
      ++i;
    }
  }
}

#define FTCHECK(x) do {FT_Error e = (x); CheckError(e);} while (false)

namespace gil = boost::gil;

namespace yg
{

  GlyphKey::GlyphKey(unsigned short id, int fontSize, bool isMask)
    : m_id(id), m_fontSize(fontSize), m_isMask(isMask)
  {}

  bool operator<(GlyphKey const & l, GlyphKey const & r)
  {
    if (l.m_id != r.m_id)
      return l.m_id < r.m_id;
    if (l.m_fontSize != r.m_fontSize)
      return l.m_fontSize < r.m_fontSize;
    return l.m_isMask < r.m_isMask;
  }

  struct GlyphCache::Impl
  {
    FT_Library m_lib;
    FT_Stroker m_stroker;

    typedef vector<FT_Face> TFontFaces;
    TFontFaces m_faces;

    typedef map<GlyphKey, shared_ptr<GlyphInfo> > TGlyphs;
    TGlyphs m_glyphs;
  };

  GlyphCache::GlyphCache() : m_impl(new Impl())
  {
    FTCHECK(FT_Init_FreeType(&m_impl->m_lib));
    FTCHECK(FT_Stroker_New(m_impl->m_lib, &m_impl->m_stroker));
    FT_Stroker_Set(m_impl->m_stroker, 2 * 64, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
  }

  GlyphCache::~GlyphCache()
  {
    for (Impl::TFontFaces::iterator it = m_impl->m_faces.begin(); it != m_impl->m_faces.end(); ++it)
     FT_Done_Face(*it);
    FT_Stroker_Done(m_impl->m_stroker);
    FT_Done_FreeType(m_impl->m_lib);
  }

  void GlyphCache::addFont(char const * fileName)
  {
    FT_Face face;
    FT_Error err = FT_New_Face(m_impl->m_lib, fileName, 0, &face);
    m_impl->m_faces.push_back(face);
    if (err == 0)
      LOG(LINFO, ("Added font ", fileName))
    else
      LOG(LINFO, ("Font wasn't added! Path: ", fileName, ", Error: ", err));
  }

  shared_ptr<GlyphInfo> const GlyphCache::getGlyph(GlyphKey const & k)
  {
    GlyphKey key(k);
    Impl::TGlyphs::const_iterator it = m_impl->m_glyphs.find(key);
    if (it != m_impl->m_glyphs.end())
      return it->second;
    else
    {
      FT_Face face = m_impl->m_faces.front();
      FTCHECK(FT_Set_Pixel_Sizes(face, 0, key.m_fontSize));

      int symbolIdx = FT_Get_Char_Index(face, key.m_id);
      if (symbolIdx == 0)
      {
        key = GlyphKey(65533, key.m_fontSize, key.m_isMask);
        it = m_impl->m_glyphs.find(key);
        if (it != m_impl->m_glyphs.end())
          return it->second;
        else
        {
          symbolIdx = FT_Get_Char_Index(face, key.m_id);
          if (symbolIdx == 0)
            throw std::exception();
        }
      }

      FTCHECK(FT_Load_Glyph(face, symbolIdx, FT_LOAD_DEFAULT));

      FT_Glyph glyph;
      FT_BitmapGlyph bitmapGlyph;

      if (key.m_isMask)
      {
        FTCHECK(FT_Get_Glyph(face->glyph, &glyph));
        FTCHECK(FT_Glyph_Stroke(&glyph, m_impl->m_stroker, 1));
        FTCHECK(FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 0));
        bitmapGlyph = (FT_BitmapGlyph)glyph;
      }
      else
      {
        FTCHECK(FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL));
        FTCHECK(FT_Get_Glyph(face->glyph, &glyph));
        bitmapGlyph = (FT_BitmapGlyph)glyph;
      }

      shared_ptr<GlyphInfo> info(new GlyphInfo());
      info->m_height = bitmapGlyph->bitmap.rows;
      info->m_width = bitmapGlyph->bitmap.width;
      info->m_xOffset = bitmapGlyph->left;
      info->m_yOffset = bitmapGlyph->top - info->m_height;
      info->m_xAdvance = int(bitmapGlyph->root.advance.x >> 16);
      info->m_color = key.m_isMask ? yg::Color(255, 255, 255, 0) : yg::Color(0, 0, 0, 0);

      if ((info->m_width != 0) && (info->m_height != 0))
      {
        info->m_bitmap.resize(info->m_width * info->m_height * 4);

        DATA_TRAITS::view_t dstView = gil::interleaved_view(
            info->m_width,
            info->m_height,
            (DATA_TRAITS::pixel_t*)&info->m_bitmap[0],
            info->m_width * sizeof(DATA_TRAITS::pixel_t)
            );

        gil::gray8c_view_t srcView = gil::interleaved_view(
            info->m_width,
            info->m_height,
            (gil::gray8_pixel_t*)bitmapGlyph->bitmap.buffer,
            bitmapGlyph->bitmap.pitch
            );

        DATA_TRAITS::pixel_t c(key.m_isMask ? DATA_TRAITS::maxChannelVal : 0,
                               key.m_isMask ? DATA_TRAITS::maxChannelVal : 0,
                               key.m_isMask ? DATA_TRAITS::maxChannelVal : 0,
                               0);

        for (size_t y = 0; y < srcView.height(); ++y)
          for (size_t x = 0; x < srcView.width(); ++x)
          {
            gil::get_color(c, gil::alpha_t()) = srcView(x, y) / DATA_TRAITS::channelScaleFactor;
            dstView(x, y) = c;
          }
      }

      FT_Done_Glyph(glyph);

      m_impl->m_glyphs[key] = info;
    }

    return m_impl->m_glyphs[key];
  }

  void GlyphInfo::dump(const char *fileName)
  {
/*    gil::lodepng_write_view(fileName,
                            gil::interleaved_view(m_width,
                                                  m_height,
                                                  (DATA_TRAITS::pixel_t*)&m_bitmap[0],
                                                  m_width * sizeof(DATA_TRAITS::pixel_t)));*/
  }
}
