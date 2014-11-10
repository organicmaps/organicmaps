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

strings::UniString FeatureTypeToString(uint32_t type);

template <class ContainerT, class DelimsT>
bool TokenizeStringAndCheckIfLastTokenIsPrefix(strings::UniString const & s,
                                               ContainerT & tokens,
                                               DelimsT const & delimiter)
{
  SplitUniString(s, MakeBackInsertFunctor(tokens), delimiter);
  return !s.empty() && !delimiter(s.back());
}


template <class ContainerT, class DelimsT>
bool TokenizeStringAndCheckIfLastTokenIsPrefix(string const & s,
                                               ContainerT & tokens,
                                               DelimsT const & delimiter)
{
  return TokenizeStringAndCheckIfLastTokenIsPrefix(NormalizeAndSimplifyString(s),
                                                   tokens,
                                                   delimiter);
}

void GetStreetName(strings::SimpleTokenizer iter, string & streetName);
void GetStreetNameAsKey(string const & name, string & res);

}  // namespace search
