#pragma once

#include "drape/drape_diagnostics.hpp"

#include "base/string_utils.hpp"

#include <list>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <unordered_set>

namespace dp
{
class GlyphUsageTracker
{
public:
  struct UnexpectedGlyphData
  {
    size_t m_counter = 0;
    size_t m_group = 0;
    std::set<size_t> m_expectedGroups;
  };
  using UnexpectedGlyphs = std::map<strings::UniChar, UnexpectedGlyphData>;
  using InvalidGlyphs = std::map<strings::UniChar, size_t>;

  struct GlyphUsageStatistic
  {
    std::string ToString() const;

    InvalidGlyphs m_invalidGlyphs;
    UnexpectedGlyphs m_unexpectedGlyphs;
  };

  static GlyphUsageTracker & Instance();

  void AddInvalidGlyph(strings::UniString const & str, strings::UniChar const & c);
  void AddUnexpectedGlyph(strings::UniString const & str, strings::UniChar const & c, size_t const group,
                          size_t const expectedGroup);

  GlyphUsageStatistic Report();

private:
  GlyphUsageTracker() = default;
  GlyphUsageTracker(GlyphUsageTracker const & rhs) = delete;
  GlyphUsageTracker(GlyphUsageTracker && rhs) = delete;

private:
  GlyphUsageStatistic m_glyphStat;
  std::unordered_set<std::string> m_processedStrings;

  std::mutex m_mutex;
};
}  // namespace dp
