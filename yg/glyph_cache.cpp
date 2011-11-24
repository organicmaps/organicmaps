#include "glyph_cache.hpp"
#include "glyph_cache_impl.hpp"
#include "data_formats.hpp"
#include "internal/opengl.hpp"
#include "ft2_debug.hpp"

#include "../3party/fribidi/lib/fribidi-deprecated.h"
#include "../3party/fribidi/lib/fribidi.h"

#include "../coding/lodepng_io.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"

#include "../std/vector.hpp"
#include "../std/map.hpp"

#include <boost/gil/gil_all.hpp>


namespace gil = boost::gil;

namespace yg
{
  GlyphKey::GlyphKey(strings::UniChar symbolCode, int fontSize, bool isMask, yg::Color const & color)
    : m_symbolCode(symbolCode), m_fontSize(fontSize), m_isMask(isMask), m_color(color)
  {}

  uint32_t GlyphKey::toUInt32() const
  {
    return static_cast<uint32_t>(m_symbolCode) << 16
         | static_cast<uint32_t>(m_fontSize) << 8
         | static_cast<uint32_t>(m_isMask);
  }

  bool operator<(GlyphKey const & l, GlyphKey const & r)
  {
    if (l.m_symbolCode != r.m_symbolCode)
      return l.m_symbolCode < r.m_symbolCode;
    if (l.m_fontSize != r.m_fontSize)
      return l.m_fontSize < r.m_fontSize;
    if (l.m_isMask != r.m_isMask)
      return l.m_isMask < r.m_isMask;
    return l.m_color < r.m_color;
  }

  GlyphInfo::GlyphInfo()
  {
  }

  GlyphInfo::~GlyphInfo()
  {
  }

  struct RawGlyphInfo : public GlyphInfo
  {
    ~RawGlyphInfo()
    {
      delete m_bitmapData;
    }
  };

  struct FTGlyphInfo : public GlyphInfo
  {
    FTC_Node m_node;
    FTC_Manager m_manager;

    FTGlyphInfo(FTC_Node node, FTC_Manager manager)
      : m_node(node), m_manager(manager)
    {}

    ~FTGlyphInfo()
    {
      FTC_Node_Unref(m_node, m_manager);
    }
  };

  GlyphCache::Params::Params(string const & blocksFile, string const & whiteListFile, string const & blackListFile, size_t maxSize)
    : m_blocksFile(blocksFile), m_whiteListFile(whiteListFile), m_blackListFile(blackListFile), m_maxSize(maxSize)
  {}

  GlyphCache::GlyphCache()
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
    vector<shared_ptr<Font> > & fonts = m_impl->getFonts(key.m_symbolCode);

    Font * font = 0;

    int charIDX;

    for (size_t i = 0; i < fonts.size(); ++i)
    {
      font = fonts[i].get();
      FTC_FaceID faceID = reinterpret_cast<FTC_FaceID>(font);

      charIDX = FTC_CMapCache_Lookup(
          m_impl->m_charMapCache,
          faceID,
          -1,
          key.m_symbolCode
          );
      if (charIDX != 0)
        return make_pair(font, charIDX);
    }

#ifdef DEBUG

    for (size_t i = 0; i < m_impl->m_unicodeBlocks.size(); ++i)
    {
      if (m_impl->m_unicodeBlocks[i].hasSymbol(key.m_symbolCode))
      {
        LOG(LINFO, ("Symbol", key.m_symbolCode, "not found, unicodeBlock=", m_impl->m_unicodeBlocks[i].m_name));
        break;
      }
    }

#endif

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

  shared_ptr<GlyphInfo> const GlyphCache::getGlyphInfo(GlyphKey const & key)
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
//    FTC_Node node;

    GlyphInfo * info = 0;

    if (key.m_isMask)
    {
      FTCHECK(FTC_StrokedImageCache_LookupScaler(
          m_impl->m_strokedGlyphCache,
          &fontScaler,
          m_impl->m_stroker,
          FT_LOAD_DEFAULT,
          charIDX.second,
          &glyph,
          0
          //&node
          ));

//      info = new FTGlyphInfo(node, m_impl->m_manager);
    }
    else
    {
      FTCHECK(FTC_ImageCache_LookupScaler(
          m_impl->m_normalGlyphCache,
          &fontScaler,
          FT_LOAD_DEFAULT | FT_LOAD_RENDER,
          charIDX.second,
          &glyph,
          0
          //&node
          ));

//      info = new FTGlyphInfo(node, m_impl->m_manager);
    }

    info = new RawGlyphInfo();

    FT_BitmapGlyph bitmapGlyph = (FT_BitmapGlyph)glyph;

    info->m_metrics.m_height = bitmapGlyph ? bitmapGlyph->bitmap.rows : 0;
    info->m_metrics.m_width = bitmapGlyph ? bitmapGlyph->bitmap.width : 0;
    info->m_metrics.m_xOffset = bitmapGlyph ? bitmapGlyph->left : 0;
    info->m_metrics.m_yOffset = bitmapGlyph ? bitmapGlyph->top - info->m_metrics.m_height : 0;
    info->m_metrics.m_xAdvance = bitmapGlyph ? int(bitmapGlyph->root.advance.x >> 16) : 0;
    info->m_metrics.m_yAdvance = bitmapGlyph ? int(bitmapGlyph->root.advance.y >> 16) : 0;
    info->m_color = key.m_color;

    info->m_bitmapData = 0;
    info->m_bitmapPitch = 0;

    if ((info->m_metrics.m_width != 0) && (info->m_metrics.m_height != 0))
    {
//      info->m_bitmapData = bitmapGlyph->bitmap.buffer;
//      info->m_bitmapPitch = bitmapGlyph->bitmap.pitch;

      info->m_bitmapPitch = bitmapGlyph->bitmap.pitch;
      info->m_bitmapData = new unsigned char[info->m_bitmapPitch * info->m_metrics.m_height];

      memcpy(info->m_bitmapData, bitmapGlyph->bitmap.buffer, info->m_bitmapPitch * info->m_metrics.m_height);
    }

    return make_shared_ptr(info);
  }

  double GlyphCache::getTextLength(double fontSize, string const & text)
  {
    strings::UniString const s = strings::MakeUniString(text);
    double len = 0;
    for (unsigned i = 0; i < s.size(); ++i)
    {
      GlyphKey k(s[i], static_cast<uint32_t>(fontSize), false, yg::Color(0, 0, 0, 255));
      len += getGlyphMetrics(k).m_xAdvance;
    }

    return len;
  }

  strings::UniString GlyphCache::log2vis(strings::UniString const & str)
  {
//    FriBidiEnv e;
    size_t const count = str.size();
    strings::UniString res(count);
    FriBidiParType dir = FRIBIDI_PAR_LTR;  // requested base direction
    fribidi_log2vis(&str[0], count, &dir, &res[0], 0, 0, 0);
    return res;
//    return str;
  }

}
