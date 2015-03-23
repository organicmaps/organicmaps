#include "glyph_usage_tracker.hpp"

#include "../../base/assert.hpp"
#include "../../std/sstream.hpp"
#include "../../platform/preferred_languages.hpp"

namespace dp
{

GlyphUsageTracker & GlyphUsageTracker::Instance()
{
  static GlyphUsageTracker s_inst;
  return s_inst;
}

string GlyphUsageTracker::Report()
{
  lock_guard<mutex> lock(m_mutex);

  ostringstream ss;
  ss << "\n ===== Glyphs Usage Report ===== \n";
  ss << " Current language = " << languages::GetCurrentOrig() << "\n";
  ss << " Invalid glyphs count = " << m_invalidGlyphs.size() << "\n";
  ss << " Invalid glyphs: { ";
  for (auto const & it : m_invalidGlyphs)
    ss << it.first << "(" << it.second << ") ";
  ss << "}\n";

  ss << " Unexpected glyphs count = " << m_unexpectedGlyphs.size() << "\n";
  ss << " Unexpected glyphs: {\n";
  for (auto const & it : m_unexpectedGlyphs)
  {
    ss << "   glyph = " << it.first << ", usages = " << it.second.counter << ", group = " << it.second.group << ", expected groups = { ";
    for (auto const & gr : it.second.expectedGroups)
      ss << gr << " ";
    ss << "}\n";
  }
  ss << " }\n";
  ss << " ===== Glyphs Usage Report ===== \n";

  return ss.str();
}

void GlyphUsageTracker::AddInvalidGlyph(strings::UniChar const & c)
{
  lock_guard<mutex> lock(m_mutex);

  auto it = m_invalidGlyphs.find(c);
  if (it != m_invalidGlyphs.end())
    it->second++;
  else
    m_invalidGlyphs.insert(make_pair(c, 1));
}

void GlyphUsageTracker::AddUnexpectedGlyph(strings::UniChar const & c, size_t const group, size_t const expectedGroup)
{
  lock_guard<mutex> lock(m_mutex);

  auto it = m_unexpectedGlyphs.find(c);
  if (it != m_unexpectedGlyphs.end())
  {
    ASSERT(it->second.group == group, (""));
    it->second.counter++;
    if (it->second.expectedGroups.find(expectedGroup) == it->second.expectedGroups.end())
      it->second.expectedGroups.emplace(expectedGroup);
  }
  else
  {
    UnexpectedGlyphData data;
    data.group = group;
    data.expectedGroups.emplace(expectedGroup);
    data.counter = 1;
    m_unexpectedGlyphs.insert(make_pair(c, data));
  }
}

} // namespace dp
