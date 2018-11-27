#pragma once

#include "indexer/search_delimiters.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

namespace search
{
// This function should be used for all search strings normalization.
// It does some magic text transformation which greatly helps us to improve our search.
strings::UniString NormalizeAndSimplifyString(std::string const & s);

template <class Delims, typename Fn>
void SplitUniString(strings::UniString const & uniS, Fn && f, Delims const & delims)
{
  for (strings::TokenizeIterator<Delims> iter(uniS, delims); iter; ++iter)
    f(iter.GetUniString());
}

template <typename Tokens, typename Delims>
void NormalizeAndTokenizeString(std::string const & s, Tokens & tokens, Delims const & delims)
{
  SplitUniString(NormalizeAndSimplifyString(s), ::base::MakeBackInsertFunctor(tokens), delims);
}

template <typename Tokens>
void NormalizeAndTokenizeString(std::string const & s, Tokens & tokens)
{
  SplitUniString(NormalizeAndSimplifyString(s), ::base::MakeBackInsertFunctor(tokens),
                 search::Delimiters());
}

template <typename Tokens>
void NormalizeAndTokenizeAsUtf8(std::string const & s, Tokens & tokens)
{
  tokens.clear();
  auto const fn = [&](strings::UniString const & s) { tokens.emplace_back(strings::ToUtf8(s)); };
  SplitUniString(NormalizeAndSimplifyString(s), fn, search::Delimiters());
}

template <typename Fn>
void ForEachNormalizedToken(std::string const & s, Fn && fn)
{
  SplitUniString(NormalizeAndSimplifyString(s), std::forward<Fn>(fn), search::Delimiters());
}

strings::UniString FeatureTypeToString(uint32_t type);

template <class Tokens, class Delims>
bool TokenizeStringAndCheckIfLastTokenIsPrefix(strings::UniString const & s,
                                               Tokens & tokens,
                                               Delims const & delims)
{
  SplitUniString(s, ::base::MakeBackInsertFunctor(tokens), delims);
  return !s.empty() && !delims(s.back());
}

template <class Tokens, class Delims>
bool TokenizeStringAndCheckIfLastTokenIsPrefix(std::string const & s, Tokens & tokens,
                                               Delims const & delims)
{
  return TokenizeStringAndCheckIfLastTokenIsPrefix(NormalizeAndSimplifyString(s), tokens, delims);
}

// Chops off the last query token (the "prefix" one) from |str|.
std::string DropLastToken(std::string const & str);

strings::UniString GetStreetNameAsKey(std::string const & name);

// *NOTE* The argument string must be normalized and simplified.
bool IsStreetSynonym(strings::UniString const & s);
bool IsStreetSynonymPrefix(strings::UniString const & s);

/// Normalizes both str and substr, and then returns true if substr is found in str.
/// Used in native platform code for search in localized strings (cuisines, categories, strings etc.).
bool ContainsNormalized(std::string const & str, std::string const & substr);

// This class can be used as a filter for street tokens.  As there can
// be street synonyms in the street name, single street synonym is
// skipped, but multiple synonyms are left as is. For example, when
// applied to ["улица", "ленина"] the filter must emit only
// ["ленина"], but when applied to ["улица", "набережная"] the filter
// must emit both tokens as is, i.e. ["улица", "набережная"].
class StreetTokensFilter
{
public:
  using Callback = std::function<void(strings::UniString const & token, size_t tag)>;

  template <typename TC>
  StreetTokensFilter(TC && callback) : m_callback(std::forward<TC>(callback))
  {
  }

  // Puts token to the filter. Filter checks following cases:
  // * when |token| is the first street synonym met so far, it's delayed
  // * when |token| is the second street synonym met so far,
  //   callback is called for the |token| and for the previously delayed token
  // * otherwise, callback is called for the |token|
  void Put(strings::UniString const & token, bool isPrefix, size_t tag);

private:
  using Cell = std::pair<strings::UniString, size_t>;

  inline void EmitToken(strings::UniString const & token, size_t tag) { m_callback(token, tag); }

  strings::UniString m_delayedToken;
  size_t m_delayedTag = 0;
  size_t m_numSynonyms = 0;

  Callback m_callback;
};
}  // namespace search
