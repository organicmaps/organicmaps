#include "base/string_utils.hpp"

#include "3party/fribidi/lib/fribidi.h"

#include "std/mutex.hpp"

namespace fribidi
{

strings::UniString log2vis(strings::UniString const & str)
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
    fribidi_log2vis(&str[0], count, &dir, &res[0], 0, 0, 0);
  }

  return res;
}

}
