#include "../base/SRC_FIRST.hpp"

#include "glyph_cache.hpp"
#include "glyph_cache_impl.hpp"
#include "data_formats.hpp"
#include "internal/opengl.hpp"
#include "ft2_debug.hpp"

#include "../coding/lodepng_io.hpp"

#include "../base/logging.hpp"

#include "../std/vector.hpp"
#include "../std/map.hpp"

#include <boost/gil/gil_all.hpp>


namespace gil = boost::gil;

namespace yg
{
  GlyphKey::GlyphKey(int id, int fontSize, bool isMask, yg::Color const & color)
    : m_id(id), m_fontSize(fontSize), m_isMask(isMask), m_color(color)
  {}

  uint32_t GlyphKey::toUInt32() const
  {
    return static_cast<uint32_t>(m_id) << 16
         | static_cast<uint32_t>(m_fontSize) << 8
         | static_cast<uint32_t>(m_isMask);
  }

  bool operator<(GlyphKey const & l, GlyphKey const & r)
  {
    if (l.m_id != r.m_id)
      return l.m_id < r.m_id;
    if (l.m_fontSize != r.m_fontSize)
      return l.m_fontSize < r.m_fontSize;
    if (l.m_isMask != r.m_isMask)
      return l.m_isMask < r.m_isMask;
    return l.m_color < r.m_color;
  }

  GlyphCache::Params::Params(char const * blocksFile, char const * whiteListFile, char const * blackListFile, size_t maxSize)
    : m_blocksFile(blocksFile), m_whiteListFile(whiteListFile), m_blackListFile(blackListFile), m_maxSize(maxSize)
  {}

  GlyphCache::GlyphCache(Params const & params) : m_impl(new GlyphCacheImpl(params))
  {
  }

  void GlyphCache::addFont(const char *fileName)
  {
    m_impl->addFont(fileName);
  }

  void GlyphCache::addFonts(vector<string> const & fontNames)
  {
    m_impl->addFonts(fontNames);
  }

  pair<Font*, int> GlyphCache::getCharIDX(GlyphKey const & key)
  {
    vector<shared_ptr<Font> > & fonts = m_impl->getFonts(key.m_id);

    Font * font = 0;

    int charIDX;

    for (int i = 0; i < fonts.size(); ++i)
    {
      font = fonts[i].get();
      FTC_FaceID faceID = reinterpret_cast<FTC_FaceID>(font);

      charIDX = FTC_CMapCache_Lookup(
          m_impl->m_charMapCache,
          faceID,
          -1,
          key.m_id
          );
      if (charIDX != 0)
        return make_pair(font, charIDX);
    }

    font = fonts.front().get();

    /// taking substitution character from the first font in the list
    charIDX = FTC_CMapCache_Lookup(
        m_impl->m_charMapCache,
        reinterpret_cast<FTC_FaceID>(font),
        -1,
        65533
        );
    if (charIDX == 0)
      charIDX = FTC_CMapCache_Lookup(
          m_impl->m_charMapCache,
          reinterpret_cast<FTC_FaceID>(font),
          -1,
          32
          );

    return make_pair(font, charIDX);
  }

  GlyphMetrics const GlyphCache::getGlyphMetrics(GlyphKey const & key)
  {
    pair<Font*, int> charIDX = getCharIDX(key);

    FTC_ScalerRec fontScaler =
    {
      reinterpret_cast<FTC_FaceID>(charIDX.first),
      key.m_fontSize,
      key.m_fontSize,
      1,
      0,
      0
    };

    FT_Glyph glyph = 0;

    FTCHECK(FTC_ImageCache_LookupScaler(
      m_impl->m_glyphMetricsCache,
      &fontScaler,
      FT_LOAD_DEFAULT,
      charIDX.second,
      &glyph,
      0));

    FT_BBox cbox;
    FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &cbox);

    GlyphMetrics m =
    {
      glyph->advance.x >> 16,
      glyph->advance.y >> 16,
      cbox.xMin, cbox.yMin,
      cbox.xMax - cbox.xMin, cbox.yMax - cbox.yMin
    };

    return m;
  }

  shared_ptr<GlyphInfo> const GlyphCache::getGlyph(GlyphKey const & key)
  {
    pair<Font *, int> charIDX = getCharIDX(key);

    FTC_ScalerRec fontScaler =
    {
      reinterpret_cast<FTC_FaceID>(charIDX.first),
      key.m_fontSize,
      key.m_fontSize,
      1,
      0,
      0
    };

    FT_Glyph glyph = 0;

    if (key.m_isMask)
      FTCHECK(FTC_ImageCache_LookupScaler(
          m_impl->m_strokedGlyphCache,
          &fontScaler,
          FT_LOAD_DEFAULT,
          charIDX.second,
          &glyph,
          0
          ));
    else
      FTCHECK(FTC_ImageCache_LookupScaler(
          m_impl->m_normalGlyphCache,
          &fontScaler,
          FT_LOAD_DEFAULT | FT_LOAD_RENDER,
          charIDX.second,
          &glyph,
          0
          ));

    FT_BitmapGlyph bitmapGlyph = (FT_BitmapGlyph)glyph;

    shared_ptr<GlyphInfo> info(new GlyphInfo());

    info->m_metrics.m_height = bitmapGlyph ? bitmapGlyph->bitmap.rows : 0;
    info->m_metrics.m_width = bitmapGlyph ? bitmapGlyph->bitmap.width : 0;
    info->m_metrics.m_xOffset = bitmapGlyph ? bitmapGlyph->left : 0;
    info->m_metrics.m_yOffset = bitmapGlyph ? bitmapGlyph->top - info->m_metrics.m_height : 0;
    info->m_metrics.m_xAdvance = bitmapGlyph ? int(bitmapGlyph->root.advance.x >> 16) : 0;
    info->m_metrics.m_yAdvance = bitmapGlyph ? int(bitmapGlyph->root.advance.y >> 16) : 0;
    info->m_color = key.m_color;

    if ((info->m_metrics.m_width != 0) && (info->m_metrics.m_height != 0))
    {
      info->m_bitmap.resize(info->m_metrics.m_width * info->m_metrics.m_height * sizeof(DATA_TRAITS::pixel_t));

      DATA_TRAITS::view_t dstView = gil::interleaved_view(
            info->m_metrics.m_width,
            info->m_metrics.m_height,
            (DATA_TRAITS::pixel_t*)&info->m_bitmap[0],
            info->m_metrics.m_width * sizeof(DATA_TRAITS::pixel_t)
            );

      gil::gray8c_view_t srcView = gil::interleaved_view(
          info->m_metrics.m_width,
          info->m_metrics.m_height,
          (gil::gray8_pixel_t*)bitmapGlyph->bitmap.buffer,
          bitmapGlyph->bitmap.pitch
          );

      DATA_TRAITS::pixel_t c;

      gil::get_color(c, gil::red_t()) = key.m_color.r / DATA_TRAITS::channelScaleFactor;
      gil::get_color(c, gil::green_t()) = key.m_color.g / DATA_TRAITS::channelScaleFactor;
      gil::get_color(c, gil::blue_t()) = key.m_color.b / DATA_TRAITS::channelScaleFactor;
      gil::get_color(c, gil::alpha_t()) = key.m_color.a / DATA_TRAITS::channelScaleFactor;

      for (size_t y = 0; y < srcView.height(); ++y)
        for (size_t x = 0; x < srcView.width(); ++x)
        {
          gil::get_color(c, gil::alpha_t()) = srcView(x, y) / DATA_TRAITS::channelScaleFactor;
          dstView(x, y) = c;
        }
    }

    return info;
  }

  void GlyphInfo::dump(const char * /*fileName */)
  {
/*    gil::lodepng_write_view(fileName,
                            gil::interleaved_view(m_width,
                                                  m_height,
                                                  (DATA_TRAITS::pixel_t*)&m_bitmap[0],
                                                  m_width * sizeof(DATA_TRAITS::pixel_t)));*/
  }
}
