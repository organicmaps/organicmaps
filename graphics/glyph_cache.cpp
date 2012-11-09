#include "glyph_cache.hpp"
#include "glyph_cache_impl.hpp"
#include "data_traits.hpp"
#include "internal/opengl.hpp"
#include "ft2_debug.hpp"

#include "../3party/fribidi/lib/fribidi-deprecated.h"
#include "../3party/fribidi/lib/fribidi.h"

#include "../coding/lodepng_io.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"

#include "../std/vector.hpp"
#include "../std/map.hpp"
#include "../base/mutex.hpp"

#include <boost/gil/gil_all.hpp>


namespace gil = boost::gil;

namespace graphics
{
  GlyphKey::GlyphKey(strings::UniChar symbolCode, int fontSize, bool isMask, graphics::Color const & color)
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

  GlyphCache::Params::Params(string const & blocksFile, string const & whiteListFile, string const & blackListFile, size_t maxSize, bool isDebugging)
    : m_blocksFile(blocksFile), m_whiteListFile(whiteListFile), m_blackListFile(blackListFile), m_maxSize(maxSize), m_isDebugging(isDebugging)
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
    return m_impl->getCharIDX(key);
  }

  GlyphMetrics const GlyphCache::getGlyphMetrics(GlyphKey const & key)
  {
    return m_impl->getGlyphMetrics(key);
  }

  shared_ptr<GlyphInfo> const GlyphCache::getGlyphInfo(GlyphKey const & key)
  {
    return m_impl->getGlyphInfo(key);
  }

  double GlyphCache::getTextLength(double fontSize, string const & text)
  {
    strings::UniString const s = strings::MakeUniString(text);
    double len = 0;
    for (unsigned i = 0; i < s.size(); ++i)
    {
      GlyphKey k(s[i], static_cast<uint32_t>(fontSize), false, graphics::Color(0, 0, 0, 255));
      len += getGlyphMetrics(k).m_xAdvance;
    }

    return len;
  }

  threads::Mutex m;

  strings::UniString GlyphCache::log2vis(strings::UniString const & str)
  {
//    FriBidiEnv e;
    threads::MutexGuard g(m);
    size_t const count = str.size();
    strings::UniString res(count);
    FriBidiParType dir = FRIBIDI_PAR_LTR;  // requested base direction
    fribidi_log2vis(&str[0], count, &dir, &res[0], 0, 0, 0);
    return res;
//    return str;
  }

}
