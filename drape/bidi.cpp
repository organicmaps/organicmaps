#include "drape/bidi.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

// ICU includes.
#include <unicode/ubidi.h>
#include <unicode/unistr.h>
#include <unicode/ushape.h>

namespace bidi
{

using StringT = strings::UniString;

StringT log2vis(StringT const & str)
{
  uint32_t const usize = static_cast<uint32_t>(str.size());

  buffer_vector<UChar, 256> ustr(usize);
  bool isAscii = true;
  bool hasExtendedChars = false;
  for (uint32_t i = 0; i < usize; ++i)
  {
    auto const c = str[i];
    if (c >= 0x80)
      isAscii = false;

    // As an example of extended chars: https://www.openstreetmap.org/node/5680098021
    if (c > 0xFFFF)
    {
      hasExtendedChars = true;
      ustr[i] = ' ';
    }
    else
      ustr[i] = c;
  }
  if (isAscii)
    return str;

  // Remove non-drawable chars if any, or drape will not draw a string otherwise.
  StringT out;
  auto const ResultString = [&str, &out, hasExtendedChars]() -> StringT const &
  {
    if (hasExtendedChars)
    {
      out = str;
      out.erase_if([](strings::UniChar c) { return c > 0xFFFF; });
      return out;
    }
    else
      return str;
  };

  UBiDi * bidi = ubidi_open();
  SCOPE_GUARD(closeBidi, [bidi]() { ubidi_close(bidi); });

  UErrorCode errorCode = U_ZERO_ERROR;

  ubidi_setPara(bidi, ustr.data(), usize, UBIDI_DEFAULT_LTR, nullptr, &errorCode);

  UBiDiDirection const direction = ubidi_getDirection(bidi);
  if (direction == UBIDI_LTR || direction == UBIDI_NEUTRAL)
    return ResultString();

  uint32_t const buffSize = usize * 2;
  buffer_vector<UChar, 256> buff(buffSize, 0);

  u_shapeArabic(ustr.data(), usize, buff.data(), buffSize,
                U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED, &errorCode);
  if (errorCode != U_ZERO_ERROR)
  {
    LOG(LWARNING, ("u_shapeArabic failed, icu error:", errorCode));
    return ResultString();
  }

  icu::UnicodeString shaped(buff.data());
  ubidi_setPara(bidi, shaped.getTerminatedBuffer(), shaped.length(), direction, nullptr, &errorCode);

  ubidi_writeReordered(bidi, buff.data(), buffSize, 0, &errorCode);
  if (errorCode != U_ZERO_ERROR)
  {
    LOG(LWARNING, ("ubidi_writeReordered failed, icu error:", errorCode));
    return ResultString();
  }

  out.reserve(buffSize);
  for (uint32_t i = 0; i < buffSize; ++i)
  {
    if (buff[i])
      out.push_back(buff[i]);
    else
      break;
  }
  return out;
}

StringT log2vis(std::string const & utf8)
{
  auto uni = strings::MakeUniString(utf8);
  if (utf8.size() == uni.size())
  {
    // obvious ASCII
    return uni;
  }
  else
    return log2vis(uni);
}

} // namespace bidi
