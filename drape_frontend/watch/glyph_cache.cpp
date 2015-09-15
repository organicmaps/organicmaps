#include "drape_frontend/watch/glyph_cache.hpp"
#include "drape_frontend/watch/glyph_cache_impl.hpp"

#include "3party/fribidi/lib/fribidi.h"

namespace df
{
namespace watch
{

GlyphKey::GlyphKey(strings::UniChar symbolCode,
                   int fontSize,
                   bool isMask,
                   dp::Color const & color)
  : m_symbolCode(symbolCode),
    m_fontSize(fontSize),
    m_isMask(isMask),
    m_color(color)
{}

GlyphKey::GlyphKey()
  : m_symbolCode(0),
    m_fontSize(),
    m_isMask(),
    m_color()
{}

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

bool operator!=(GlyphKey const & l, GlyphKey const & r)
{
  return (l.m_symbolCode != r.m_symbolCode)
      || (l.m_fontSize != r.m_fontSize)
      || (l.m_isMask != r.m_isMask)
      || !(l.m_color == r.m_color);
}

GlyphCache::Params::Params(string const & blocksFile,
                           string const & whiteListFile,
                           string const & blackListFile,
                           size_t maxSize,
                           double visualScale,
                           bool isDebugging)
  : m_blocksFile(blocksFile),
    m_whiteListFile(whiteListFile),
    m_blackListFile(blackListFile),
    m_maxSize(maxSize),
    m_visualScale(visualScale),
    m_isDebugging(isDebugging)
{}

GlyphCache::GlyphCache()
{}

GlyphCache::GlyphCache(Params const & params) : m_impl(new GlyphCacheImpl(params))
{
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

shared_ptr<GlyphBitmap> const GlyphCache::getGlyphBitmap(GlyphKey const & key)
{
  return m_impl->getGlyphBitmap(key);
}

double GlyphCache::getTextLength(double fontSize, string const & text)
{
  strings::UniString const s = strings::MakeUniString(text);
  double len = 0;
  for (unsigned i = 0; i < s.size(); ++i)
  {
    GlyphKey k(s[i], static_cast<uint32_t>(fontSize), false, dp::Color(0, 0, 0, 255));
    len += getGlyphMetrics(k).m_xAdvance;
  }

  return len;
}

threads::Mutex GlyphCache::s_fribidiMutex;

strings::UniString GlyphCache::log2vis(strings::UniString const & str)
{
  size_t const count = str.size();
  if (count == 0)
    return str;

  strings::UniString res(count);

  //FriBidiEnv env;
  threads::MutexGuard g(s_fribidiMutex);
  FriBidiParType dir = FRIBIDI_PAR_LTR;  // requested base direction
  fribidi_log2vis(&str[0], count, &dir, &res[0], 0, 0, 0);
  return res;
}

}
}
