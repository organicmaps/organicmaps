#pragma once
#include "../base/string_utils.hpp"
#include "../base/base.hpp"

namespace search
{

using strings::UniChar;

inline strings::UniString NormalizeAndSimplifyString(string const & s)
{
  strings::UniString uniS = strings::MakeLowerCase(strings::MakeUniString(s));
  strings::Normalize(uniS);
  return uniS;
}

template <class DelimsT, typename F>
void SplitUniString(strings::UniString const & uniS, F f, DelimsT const & delims)
{
  for (strings::TokenizeIterator<DelimsT> iter(uniS, delims); iter; ++iter)
    f(iter.GetUniString());
}

}  // namespace search
