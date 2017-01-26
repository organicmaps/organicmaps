#pragma once

#include "drape/drape_diagnostics.hpp"

#include "base/string_utils.hpp"

#include "std/map.hpp"
#include "std/set.hpp"
#include "std/list.hpp"
#include "std/mutex.hpp"
#include "std/string.hpp"
#include "std/unordered_set.hpp"

namespace dp
{

class GlyphUsageTracker
{
public:
  struct UnexpectedGlyphData
  {
    size_t m_counter = 0;
    size_t m_group = 0;
    set<size_t> m_expectedGroups;
  };
  using UnexpectedGlyphs = map<strings::UniChar, UnexpectedGlyphData>;
  using InvalidGlyphs = map<strings::UniChar, size_t>;

  struct GlyphUsageStatistic
  {
    string ToString() const;

    InvalidGlyphs m_invalidGlyphs;
    UnexpectedGlyphs m_unexpectedGlyphs;
  };

  static GlyphUsageTracker & Instance();

  void AddInvalidGlyph(strings::UniString const & str, strings::UniChar const & c);
  void AddUnexpectedGlyph(strings::UniString const & str, strings::UniChar const & c,
                          size_t const group, size_t const expectedGroup);

  GlyphUsageStatistic Report();

private:
  GlyphUsageTracker() = default;
  GlyphUsageTracker(GlyphUsageTracker const & rhs) = delete;
  GlyphUsageTracker(GlyphUsageTracker && rhs) = delete;

private:
  GlyphUsageStatistic m_glyphStat;
  unordered_set<string> m_processedStrings;

  mutex m_mutex;
};

} // namespace dp
