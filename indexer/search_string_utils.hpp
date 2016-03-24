#pragma once
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"

namespace search
{

// This function should be used for all search strings normalization.
// It does some magic text transformation which greatly helps us to improve our search.
strings::UniString NormalizeAndSimplifyString(string const & s);

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

strings::UniString GetStreetNameAsKey(string const & name);

bool IsStreetSynonym(strings::UniString const & s);
bool IsStreetSynonymPrefix(strings::UniString const & s);

/// Normalizes both str and substr, and then returns true if substr is found in str.
/// Used in native platform code for search in localized strings (cuisines, categories, strings etc.).
bool ContainsNormalized(string const & str, string const & substr);
}  // namespace search
