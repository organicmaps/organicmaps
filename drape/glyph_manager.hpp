#pragma once

#include "base/shared_buffer_manager.hpp"
#include "base/string_utils.hpp"

#include "drape/glyph.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace dp
{
struct UnicodeBlock;

class GlyphManager
{
public:
  struct Params
  {
    std::string m_uniBlocks;
    std::string m_whitelist;
    std::string m_blacklist;

    std::vector<std::string> m_fonts;

    uint32_t m_baseGlyphHeight = 22;
  };

  explicit GlyphManager(Params const & params);
  ~GlyphManager();

  Glyph GetGlyph(strings::UniChar unicodePoints);

  void MarkGlyphReady(Glyph const & glyph);
  bool AreGlyphsReady(strings::UniString const & str) const;

  Glyph const & GetInvalidGlyph() const;

  uint32_t GetBaseGlyphHeight() const;

private:
  int GetFontIndex(strings::UniChar unicodePoint);
  // Immutable version can be called from any thread and doesn't require internal synchronization.
  int GetFontIndexImmutable(strings::UniChar unicodePoint) const;
  int FindFontIndexInBlock(UnicodeBlock const & block, strings::UniChar unicodePoint) const;

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
}  // namespace dp
