#pragma once

#include <string>
#include <vector>

#include <hb.h>

namespace harfbuzz_shaping
{
struct TextSegment
{
  // TODO(AB): Use 1 byte or 2 bytes for memory-efficient caching.
  int32_t m_start;  // Offset to the segment start in the string.
  int32_t m_length;
  hb_script_t m_script;
  hb_direction_t m_direction;

  TextSegment(int32_t start, int32_t length, hb_script_t script, hb_direction_t direction)
    : m_start(start)
    , m_length(length)
    , m_script(script)
    , m_direction(direction)
  {}
};

struct TextSegments
{
  std::u16string m_text;
  std::vector<TextSegment> m_segments;
  // TODO(AB): Reverse indexes to order segments instead of moving them.
  // std::vector<size_t> m_segmentsOrder;
};

// Finds segments with common properties suitable for harfbuzz in a single line of text without newlines.
// Any line breaking/trimming should be done by the caller.
TextSegments GetTextSegments(std::string_view utf8);

std::string DebugPrint(TextSegment const & segment);
}  // namespace harfbuzz_shaping
