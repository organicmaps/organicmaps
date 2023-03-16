#pragma once

#include "drape/drape_global.hpp"

#include "base/string_utils.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace software_renderer
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
  std::vector<unsigned char> m_data;
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
  std::shared_ptr<GlyphCacheImpl> m_impl;

public:
  struct Params
  {
    std::string m_blocksFile;
    std::string m_whiteListFile;
    std::string m_blackListFile;
    size_t m_maxSize;
    double m_visualScale;
    bool m_isDebugging;
    Params(std::string const & blocksFile,
           std::string const & whiteListFile,
           std::string const & blackListFile,
           size_t maxSize,
           double visualScale,
           bool isDebugging);
  };

  GlyphCache();
  GlyphCache(Params const & params);

  void reset();
  void addFonts(std::vector<std::string> const & fontNames);

  std::pair<Font*, int> getCharIDX(GlyphKey const & key);

  std::shared_ptr<GlyphBitmap> const getGlyphBitmap(GlyphKey const & key);
  /// return control box(could be slightly larger than the precise bound box).
  GlyphMetrics const getGlyphMetrics(GlyphKey const & key);

  double getTextLength(double fontSize, std::string const & text);
};
}  // namespace software_renderer
