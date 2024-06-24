#include "drape/harfbuzz_shaping.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <array>
#include <sstream>
#include <string>

#include <unicode/ubidi.h>    // ubidi_open, ubidi_setPara
#include <unicode/uscript.h>  // UScriptCode
#include <utf8/unchecked.h>

namespace harfbuzz_shaping
{
namespace
{
// Some Unicode characters may be a part of up to 32 different scripts.
using TScriptsArray = std::array<UScriptCode, 32>;

// Writes the script and the script extensions of the Unicode codepoint.
// Returns the number of written scripts.
size_t GetScriptExtensions(char32_t codepoint, TScriptsArray & scripts)
{
  // Fill scripts with the script extensions.
  UErrorCode icu_error = U_ZERO_ERROR;
  size_t const count = uscript_getScriptExtensions(static_cast<UChar32>(codepoint), scripts.data(),
                                                   scripts.max_size(), &icu_error);
  if (U_FAILURE(icu_error))
  {
    LOG(LWARNING, ("uscript_getScriptExtensions failed with error", icu_error));
    return 0;
  }

  return count;
}

// Intersects the script extensions set of codepoint with scripts and returns the updated size of the scripts.
// The output result will be a subset of the input result (thus resultSize can only be smaller).
size_t ScriptSetIntersect(char32_t codepoint, TScriptsArray & inOutScripts, size_t inOutScriptsCount)
{
  // Each codepoint has a Script property and a Script Extensions (Scx) property.
  //
  // The implicit Script property values 'Common' and 'Inherited' indicate that a codepoint is widely used in many
  // scripts, rather than being associated to a specific script.
  //
  // However, some codepoints that are assigned a value of 'Common' or 'Inherited' are not commonly used with all
  // scripts, but rather only with a limited set of scripts. The Script Extension property is used to specify the set
  // of script which borrow the codepoint.
  //
  // Calls to GetScriptExtensions(...) return the set of scripts where the codepoints can be used.
  // (see table 7 from http://www.unicode.org/reports/tr24/tr24-29.html)
  //
  //     Script       Script Extensions ->  Results
  //  1) Common       {Common}          ->  {Common}
  //     Inherited    {Inherited}       ->  {Inherited}
  //  2) Latin        {Latn}            ->  {Latn}
  //     Inherited    {Latn}            ->  {Latn}
  //  3) Common       {Hira Kana}       ->  {Hira Kana}
  //     Inherited    {Hira Kana}       ->  {Hira Kana}
  //  4) Devanagari   {Deva Dogr Kthi Mahj}  ->  {Deva Dogr Kthi Mahj}
  //     Myanmar      {Cakm Mymr Tale}  ->  {Cakm Mymr Tale}
  //
  // For most of the codepoints, the script extensions set contains only one element. For CJK codepoints, it's common
  // to see 3-4 scripts. For really rare cases, the set can go above 20 scripts.
  TScriptsArray codepointScripts;
  size_t const codepointScriptsCount = GetScriptExtensions(codepoint, codepointScripts);

  // Implicit script 'inherited' is inheriting scripts from preceding codepoint.
  if (codepointScriptsCount == 1 && codepointScripts[0] == USCRIPT_INHERITED)
    return inOutScriptsCount;

  auto const contains = [&codepointScripts, codepointScriptsCount](UScriptCode code)
  {
    for (size_t i = 0; i < codepointScriptsCount; ++i)
      if (codepointScripts[i] == code)
        return true;

    return false;
  };

  // Intersect both script sets.
  ASSERT(!contains(USCRIPT_INHERITED), ());
  size_t outSize = 0;
  for (size_t i = 0; i < inOutScriptsCount; ++i)
  {
    auto const currentScript = inOutScripts[i];
    if (contains(currentScript))
      inOutScripts[outSize++] = currentScript;
  }

  return outSize;
}

// Find the longest sequence of characters from 0 and up to length that have at least one common UScriptCode value.
// Writes the common script value to script and returns the length of the sequence. Takes the characters' script
// extensions into account. http://www.unicode.org/reports/tr24/#ScriptX
//
// Consider 3 characters with the script values {Kana}, {Hira, Kana}, {Kana}. Without script extensions only the first
// script in each set would be taken into account, resulting in 3 segments where 1 would be enough.
size_t ScriptInterval(std::u16string const & text, int32_t start, size_t length, UScriptCode & outScript)
{
  ASSERT_GREATER(length, 0U, ());

  auto const begin = text.begin() + start;
  auto const end = text.begin() + start + static_cast<int32_t>(length);
  auto iterator = begin;

  auto c32 = utf8::unchecked::next16(iterator);

  TScriptsArray scripts;
  size_t scriptsSize = GetScriptExtensions(c32, scripts);

  while (iterator != end)
  {
    c32 = utf8::unchecked::next16(iterator);
    scriptsSize = ScriptSetIntersect(c32, scripts, scriptsSize);
    if (scriptsSize == 0U)
    {
      length = iterator - begin - 1;
      break;
    }
  }

  outScript = scripts[0];
  return length;
}

// A copy of hb_icu_script_to_script to avoid direct ICU dependency.
hb_script_t ICUScriptToHarfbuzzScript(UScriptCode script)
{
  if (script == USCRIPT_INVALID_CODE)
    return HB_SCRIPT_INVALID;
  return hb_script_from_string(uscript_getShortName (script), -1);
}

void GetSingleTextLineRuns(TextSegments & segments)
{
  auto const & text = segments.m_text;
  auto const textLength = static_cast<int32_t>(text.length());

  // Deliberately not checking for nullptr.
  thread_local UBiDi * const bidi = ubidi_open();
  UErrorCode error = U_ZERO_ERROR;
  ::ubidi_setPara(bidi, text.data(), textLength, UBIDI_DEFAULT_LTR, nullptr, &error);
  if (U_FAILURE(error))
  {
    LOG(LERROR, ("ubidi_setPara failed with code", error));
    segments.m_segments.emplace_back(0, 0, HB_SCRIPT_UNKNOWN, HB_DIRECTION_INVALID);
    return;
  }

  // Split the original text by logical runs, then each logical run by common script and each sequence at special
  // characters and style boundaries. This invariant holds: bidiRunStart <= scriptRunStart <= breakingRunStart
  // <= breakingRunEnd <= scriptRunStart <= bidiRunEnd. AB: Breaking runs are dropped now, they may not be needed.
  for (int32_t bidiRunStart = 0; bidiRunStart < textLength;)
  {
    // Determine the longest logical run (e.g. same bidi direction) from this point.
    int32_t bidiRunBreak = 0;
    UBiDiLevel bidiLevel = 0;
    ::ubidi_getLogicalRun(bidi, bidiRunStart, &bidiRunBreak, &bidiLevel);
    int32_t const bidiRunEnd = bidiRunBreak;
    ASSERT_LESS(bidiRunStart, bidiRunEnd, ());

    for (int32_t scriptRunStart = bidiRunStart; scriptRunStart < bidiRunEnd;)
    {
      // Find the longest sequence of characters that have at least one common UScriptCode value.
      UScriptCode script = USCRIPT_INVALID_CODE;
      size_t const scriptRunEnd = ScriptInterval(segments.m_text, scriptRunStart, bidiRunEnd - scriptRunStart, script) + scriptRunStart;
      ASSERT_LESS(scriptRunStart, base::asserted_cast<int32_t>(scriptRunEnd), ());

      // TODO(AB): May need to break on different unicode blocks, parentheses, and control chars (spaces).

      // TODO(AB): Support vertical layouts if necessary.
      segments.m_segments.emplace_back(scriptRunStart, scriptRunEnd - scriptRunStart, ICUScriptToHarfbuzzScript(script),
                                   bidiLevel & 0x01 ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);

      // Move to the next script sequence.
      scriptRunStart = static_cast<int32_t>(scriptRunEnd);
    }
    // Move to the next direction sequence.
    bidiRunStart = bidiRunEnd;
  }
}

void ReorderRTL(TextSegments & segments)
{
  // TODO(AB): Optimize implementation to use indexes to segments instead of copying them.
  auto it = segments.m_segments.begin();
  auto const end = segments.m_segments.end();
  // TODO(AB): Line (default rendering) direction is determined by the first segment. It should be defined as
  // a parameter depending on the language.
  auto const lineDirection = it->m_direction;
  while (it != end)
  {
    if (it->m_direction == lineDirection)
      ++it;
    else
    {
      auto const start = it++;
      while (it != end && it->m_direction != lineDirection)
        ++it;
      std::reverse(start, it);
    }
  }
  if (lineDirection != HB_DIRECTION_LTR)
    std::reverse(segments.m_segments.begin(), end);
}
}  // namespace

TextSegments GetTextSegments(std::string_view utf8)
{
  ASSERT(!utf8.empty(), ("Shaping of empty strings is not supported"));
  ASSERT(std::string::npos == utf8.find_first_of("\r\n"), ("Shaping with line breaks is not supported", utf8));

  // TODO(AB): Can unnecessary conversion/allocation be avoided?
  TextSegments segments {strings::ToUtf16(utf8), {}};
  // TODO(AB): Runs are not split by breaking chars and by different fonts.
  GetSingleTextLineRuns(segments);
  ReorderRTL(segments);
  return segments;
}

std::string DebugPrint(TextSegment const & segment)
{
  std::stringstream ss;
  ss << "TextSegment[start=" << segment.m_start << ", length=" << segment.m_length
     << ", script=" << segment.m_script << ", direction=" << segment.m_direction << ']';
  return ss.str();
}

}  // namespace harfbuzz_shaping
