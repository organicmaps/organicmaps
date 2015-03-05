#include "base/string_utils.hpp"

#include "3party/fribidi/lib/fribidi.h"

namespace fribidi
{

strings::UniString log2vis(strings::UniString const & str)
{
  size_t const count = str.size();
  if (count == 0)
    return str;

  strings::UniString res(count);

  FriBidiParType dir = FRIBIDI_PAR_LTR;  // requested base direction
  fribidi_log2vis(&str[0], count, &dir, &res[0], 0, 0, 0);
  return res;
}

}
