#include "drape/fribidi.hpp"

#include "3party/icu/common/unicode/ubidi.h"
#include "3party/icu/common/unicode/unistr.h"
#include "3party/icu/common/unicode/ushape.h"

#include "3party/fribidi/lib/fribidi.h"
#include "std/mutex.hpp"

namespace bidi
{

strings::UniString log2vis(strings::UniString const & str)
{
  std::string str8 = strings::ToUtf8(str);
  if (strings::IsASCIIString(str8))
    return str;

  UBiDi * bidi = ubidi_open();
  UErrorCode errorCode = U_ZERO_ERROR;

  UnicodeString ustr(str8.c_str());
  ubidi_setPara(bidi, ustr.getTerminatedBuffer(), ustr.length(), UBIDI_DEFAULT_LTR, nullptr, &errorCode);

  UBiDiDirection const direction = ubidi_getDirection(bidi);
  if (direction == UBIDI_LTR || direction == UBIDI_NEUTRAL)
  {
    ubidi_close(bidi);
    return str;
  }

  std::vector<UChar> buff(ustr.length() * 2, 0);

  u_shapeArabic(ustr.getTerminatedBuffer(), ustr.length(), buff.data(), static_cast<uint32_t>(buff.size()),
                U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED, &errorCode);
  if (errorCode != U_ZERO_ERROR)
    return str;

  UnicodeString shaped(buff.data());

  ubidi_setPara(bidi, shaped.getTerminatedBuffer(), shaped.length(), direction, nullptr, &errorCode);

  ubidi_writeReordered(bidi, buff.data(), static_cast<uint32_t>(buff.size()), 0, &errorCode);
  if (errorCode != U_ZERO_ERROR)
    return str;

  UnicodeString reordered(buff.data());

  ubidi_close(bidi);

  std::string out;
  reordered.toUTF8String(out);

  return strings::MakeUniString(out);
}

strings::UniString log2vis_old(strings::UniString const & str)
{
  static mutex log2visMutex;

  size_t const count = str.size();
  if (count == 0)
    return str;

  strings::UniString res(count);

  FriBidiParType dir = FRIBIDI_PAR_LTR;  // requested base direction

  // call fribidi_log2vis synchronously
  {
    lock_guard<mutex> lock(log2visMutex);
    fribidi_log2vis(&str[0], static_cast<int>(count), &dir, &res[0], 0, 0, 0);
  }

  return res;
}

}
