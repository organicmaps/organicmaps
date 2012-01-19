#pragma once

#include "color.hpp"

#include "../base/string_utils.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/vector.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"

namespace yg
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

  /// full info about single glyph
  struct GlyphInfo
  {
  private:

    /// copying is prohibited
    GlyphInfo(GlyphInfo const &);
    GlyphInfo & operator=(GlyphInfo const &);

  public:

    GlyphMetrics m_metrics;
    yg::Color m_color;

    /// glyph bitmap in 8bpp grayscale format
    unsigned char * m_bitmapData;
    int m_bitmapPitch;

    GlyphInfo();
    virtual ~GlyphInfo();
  };

  struct GlyphKey
  {
    strings::UniChar m_symbolCode;
    int m_fontSize;
    bool m_isMask;
    yg::Color m_color;
    /// as it's used for fixed fonts only, the color doesn't matter
    /// @TODO REMOVE IT!!! All chars are already 32bit
    uint32_t toUInt32() const;
    GlyphKey(strings::UniChar symbolCode, int fontSize, bool isMask, yg::Color const & color);
  };

  struct Font;

  bool operator<(GlyphKey const & l, GlyphKey const & r);

  struct GlyphCacheImpl;

  class GlyphCache
  {
  private:

    shared_ptr<GlyphCacheImpl> m_impl;

  public:

    struct Params
    {
      string m_blocksFile;
      string m_whiteListFile;
      string m_blackListFile;
      size_t m_maxSize;
      bool   m_isDebugging;
      Params(string const & blocksFile,
             string const & whiteListFile,
             string const & blackListFile,
             size_t maxSize,
             bool isDebugging);
    };

    GlyphCache();
    GlyphCache(Params const & params);

    void reset();
    void addFont(char const * fileName);
    void addFonts(vector<string> const & fontNames);

    pair<Font*, int> getCharIDX(GlyphKey const & key);

    shared_ptr<GlyphInfo> const getGlyphInfo(GlyphKey const & key);
    /// return control box(could be slightly larger than the precise bound box).
    GlyphMetrics const getGlyphMetrics(GlyphKey const & key);

    double getTextLength(double fontSize, string const & text);

    static strings::UniString log2vis(strings::UniString const & str);

  };
}
