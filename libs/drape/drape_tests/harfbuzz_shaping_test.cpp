#include "drape/harfbuzz_shaping.hpp"
#include "testing/testing.hpp"

namespace harfbuzz_shaping
{

bool operator==(TextSegment const & s1, TextSegment const & s2)
{
  return s1.m_start == s2.m_start && s1.m_length == s2.m_length && s1.m_script == s2.m_script &&
         s1.m_direction == s2.m_direction;
}

UNIT_TEST(GetTextSegments)
{
  auto const [text, segments] = GetTextSegments("Map data © OpenStreetMap");
  TEST(text == u"Map data © OpenStreetMap", ());

  std::vector<TextSegment> const expected{
      {0, 3, HB_SCRIPT_LATIN, HB_DIRECTION_LTR},   {3, 1, HB_SCRIPT_COMMON, HB_DIRECTION_LTR},
      {4, 4, HB_SCRIPT_LATIN, HB_DIRECTION_LTR},   {8, 3, HB_SCRIPT_COMMON, HB_DIRECTION_LTR},
      {11, 13, HB_SCRIPT_LATIN, HB_DIRECTION_LTR},
  };

  TEST_EQUAL(segments, expected, ());
}
}  // namespace harfbuzz_shaping
