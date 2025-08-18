#include "drape/utils/glyph_usage_tracker.hpp"

#include "platform/preferred_languages.hpp"

#include "base/assert.hpp"

#include <sstream>

namespace dp
{
std::string GlyphUsageTracker::GlyphUsageStatistic::ToString() const
{
  std::ostringstream ss;
  ss << " ----- Glyphs usage report ----- \n";
  ss << " Current language = " << languages::GetCurrentOrig() << "\n";
  ss << " Invalid glyphs count = " << m_invalidGlyphs.size() << "\n";
  ss << " Invalid glyphs: { ";
  for (auto const & it : m_invalidGlyphs)
    ss << std::hex << static_cast<uint32_t>(it.first) << std::dec << "(" << it.second << ") ";
  ss << "}\n";

  ss << " Unexpected glyphs count = " << m_unexpectedGlyphs.size() << "\n";
  ss << " Unexpected glyphs: {\n";
  for (auto const & it : m_unexpectedGlyphs)
  {
    ss << "   glyph = " << std::hex << static_cast<uint32_t>(it.first) << std::dec
       << ", unique usages = " << it.second.m_counter << ", group = " << it.second.m_group << ", expected groups = { ";

    for (auto const & gr : it.second.m_expectedGroups)
      ss << gr << " ";
    ss << "}\n";
  }
  ss << " }\n";
  ss << " ----- Glyphs usage report ----- \n";

  return ss.str();
}

GlyphUsageTracker & GlyphUsageTracker::Instance()
{
  static GlyphUsageTracker s_inst;
  return s_inst;
}

GlyphUsageTracker::GlyphUsageStatistic GlyphUsageTracker::Report()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_glyphStat;
}

void GlyphUsageTracker::AddInvalidGlyph(strings::UniString const & str, strings::UniChar const & c)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_processedStrings.find(strings::ToUtf8(str)) != m_processedStrings.end())
    return;

  ++m_glyphStat.m_invalidGlyphs[c];

  m_processedStrings.insert(strings::ToUtf8(str));
}

void GlyphUsageTracker::AddUnexpectedGlyph(strings::UniString const & str, strings::UniChar const & c,
                                           size_t const group, size_t const expectedGroup)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_processedStrings.find(strings::ToUtf8(str)) != m_processedStrings.end())
    return;

  UnexpectedGlyphData & data = m_glyphStat.m_unexpectedGlyphs[c];
  ++data.m_counter;
  data.m_expectedGroups.emplace(expectedGroup);
  data.m_group = group;

  m_processedStrings.insert(strings::ToUtf8(str));
}
}  // namespace dp
