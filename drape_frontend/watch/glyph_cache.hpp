#pragma once

#include "drape/drape_global.hpp"

#include "base/string_utils.hpp"
#include "base/mutex.hpp"

#include "std/shared_ptr.hpp"
#include "std/vector.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"

namespace df
{
namespace watch
{

/// metrics of the single glyph
struct GlyphMetrics
{
  int m_xAdvance;
  int m_yAdvance;
  int m_xOffset;
  int m_yOffset;
  int m_width;
  int m_height;
};

struct GlyphBitmap
{
  unsigned m_width;
  unsigned m_height;
  unsigned m_pitch;
  vector<unsigned char> m_data;
};

struct GlyphKey
{
  strings::UniChar m_symbolCode;
  int m_fontSize;
  bool m_isMask;
  dp::Color m_color;

  GlyphKey(strings::UniChar symbolCode,
           int fontSize,
           bool isMask,
           dp::Color const & color);
  GlyphKey();
};

struct Font;

bool operator<(GlyphKey const & l, GlyphKey const & r);
bool operator!=(GlyphKey const & l, GlyphKey const & r);

struct GlyphCacheImpl;

class GlyphCache
{
private:

  shared_ptr<GlyphCacheImpl> m_impl;

  static threads::Mutex s_fribidiMutex;

public:

  struct Params
  {
    string m_blocksFile;
    string m_whiteListFile;
    string m_blackListFile;
    size_t m_maxSize;
    double m_visualScale;
    bool m_isDebugging;
    Params(string const & blocksFile,
           string const & whiteListFile,
           string const & blackListFile,
           size_t maxSize,
           double visualScale,
           bool isDebugging);
  };

  GlyphCache();
  GlyphCache(Params const & params);

  void reset();
  void addFonts(vector<string> const & fontNames);

  pair<Font*, int> getCharIDX(GlyphKey const & key);

  shared_ptr<GlyphBitmap> const getGlyphBitmap(GlyphKey const & key);
  /// return control box(could be slightly larger than the precise bound box).
  GlyphMetrics const getGlyphMetrics(GlyphKey const & key);

  double getTextLength(double fontSize, string const & text);

  static strings::UniString log2vis(strings::UniString const & str);
};

}
}
