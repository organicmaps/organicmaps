#pragma once

#include "search/common.hpp"
#include "search/query_params.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

class CategoriesHolder;

namespace feature
{
class TypesHolder;
}

namespace search
{
class QueryParams;
class TokenSlice;

class CategoriesInfo
{
public:
  CategoriesInfo(feature::TypesHolder const & holder, TokenSlice const & tokens,
                 Locales const & locales, CategoriesHolder const & categories);

  size_t GetMatchedLength() const { return m_matchedLength; }

  /// @return true when all tokens correspond to categories in |holder|.
  bool IsPureCategories() const { return m_pureCategories; }

  /// @return true when all tokens are categories tokens but none of them correspond to categories in |holder|.
  bool IsFalseCategories() const { return m_falseCategories; }

private:
  size_t m_matchedLength = 0;
  bool m_pureCategories = false;
  bool m_falseCategories = false;
};

struct ErrorsMade
{
  static uint16_t constexpr kInfiniteErrors = std::numeric_limits<uint16_t>::max();

  ErrorsMade() = default;
  explicit ErrorsMade(uint16_t errorsMade) : m_errorsMade(errorsMade) {}

  bool IsValid() const { return m_errorsMade != kInfiniteErrors; }
  bool IsZero() const { return m_errorsMade == 0; }
  bool IsBetterThan(ErrorsMade rhs) const { return m_errorsMade < rhs.m_errorsMade; }

  template <typename Fn>
  static ErrorsMade Combine(ErrorsMade const & lhs, ErrorsMade const & rhs, Fn && fn)
  {
    if (!lhs.IsValid())
      return rhs;
    if (!rhs.IsValid())
      return lhs;
    return ErrorsMade(fn(lhs.m_errorsMade, rhs.m_errorsMade));
  }

  static ErrorsMade Min(ErrorsMade const & lhs, ErrorsMade const & rhs)
  {
    return Combine(lhs, rhs, [](uint16_t u, uint16_t v) { return std::min(u, v); });
  }

  static ErrorsMade Max(ErrorsMade const & lhs, ErrorsMade const & rhs)
  {
    return Combine(lhs, rhs, [](uint16_t u, uint16_t v) { return std::max(u, v); });
  }

  friend ErrorsMade operator+(ErrorsMade const & lhs, ErrorsMade const & rhs)
  {
    return Combine(lhs, rhs, [](uint16_t u, uint16_t v) { return u + v; });
  }

  ErrorsMade & operator+=(ErrorsMade const & rhs)
  {
    *this = *this + rhs;
    return *this;
  }

  bool operator==(ErrorsMade const & rhs) const { return m_errorsMade == rhs.m_errorsMade; }

  uint16_t m_errorsMade = kInfiniteErrors;
};

std::string DebugPrint(ErrorsMade const & errorsMade);

namespace impl
{
// Returns the minimum number of errors needed to match |text| with |token|.
// If it's not possible in accordance with GetMaxErrorsForToken(|text|), returns kInfiniteErrors.
ErrorsMade GetErrorsMade(QueryParams::Token const & token, strings::UniString const & text);
ErrorsMade GetPrefixErrorsMade(QueryParams::Token const & token, strings::UniString const & text);
}  // namespace impl

// The order and numeric values are important here. Please, check all use-cases before changing this enum.
enum class NameScore : uint8_t
{
  // example name = "Carrefour Mini"
                    // example query:
  ZERO = 0,         // Rewe
  SUBSTRING = 1,    // Mini
  PREFIX = 2,       // Carref
  FULL_PREFIX = 3,  // Carrefour
  FULL_MATCH = 4,   // Carrefour Mini

  COUNT
};

struct NameScores
{
  NameScores() = default;
  NameScores(NameScore nameScore, ErrorsMade const & errorsMade, bool isAltOrOldName, size_t matchedLength)
    : m_nameScore(nameScore), m_errorsMade(errorsMade), m_isAltOrOldName(isAltOrOldName), m_matchedLength(matchedLength)
  {
  }

  void UpdateIfBetter(NameScores const & rhs)
  {
    auto const newNameScoreIsBetter = m_nameScore < rhs.m_nameScore;
    auto const nameScoresAreEqual = m_nameScore == rhs.m_nameScore;
    auto const newLanguageIsBetter = m_isAltOrOldName && !rhs.m_isAltOrOldName;
    auto const languagesAreEqual = m_isAltOrOldName == rhs.m_isAltOrOldName;
    auto const newMatchedLengthIsBetter = m_matchedLength < rhs.m_matchedLength;
    // It's okay to pick a slightly worse matched length if other scores are better.
    auto const matchedLengthsAreSimilar = (m_matchedLength - m_matchedLength / 4) <= rhs.m_matchedLength;

    if (newMatchedLengthIsBetter ||
       (matchedLengthsAreSimilar && newNameScoreIsBetter) ||
       (matchedLengthsAreSimilar && nameScoresAreEqual && newLanguageIsBetter))
    {
      m_nameScore = rhs.m_nameScore;
      m_errorsMade = rhs.m_errorsMade;
      m_isAltOrOldName = rhs.m_isAltOrOldName;
      m_matchedLength = rhs.m_matchedLength;
      return;
    }
    if (matchedLengthsAreSimilar && nameScoresAreEqual && languagesAreEqual)
      m_errorsMade = ErrorsMade::Min(m_errorsMade, rhs.m_errorsMade);
  }

  bool operator==(NameScores const & rhs)
  {
    return m_nameScore == rhs.m_nameScore && m_errorsMade == rhs.m_errorsMade &&
           m_isAltOrOldName == rhs.m_isAltOrOldName && m_matchedLength == rhs.m_matchedLength;
  }

  NameScore m_nameScore = NameScore::ZERO;
  ErrorsMade m_errorsMade;
  bool m_isAltOrOldName = false;
  size_t m_matchedLength = 0;
};

std::string DebugPrint(NameScore const & score);
std::string DebugPrint(NameScores const & scores);

// Returns true when |s| is a stop-word and may be removed from a query.
bool IsStopWord(strings::UniString const & s);

// Normalizes, simplifies and splits string, removes stop-words.
void PrepareStringForMatching(std::string_view name, std::vector<strings::UniString> & tokens);

/// @param[in]  tokens  Feature's name (splitted on tokens, without delimiters) to match.
/// @param[in]  slice   Input query.
template <typename Slice>
NameScores GetNameScores(std::vector<strings::UniString> const & tokens, uint8_t lang,
                         Slice const & slice)
{
  if (slice.Empty())
    return {};

  NameScores scores;
  size_t const tokenCount = tokens.size();
  size_t const sliceCount = slice.Size();

  // Try matching words between token and slice, iterating over offsets.
  // We want to try all possible offsets of the slice and token lists
  // When offset = 0, the last token in tokens is compared to the first in slice.
  // When offset = sliceCount + tokenCount, the last token
  // in slice is compared to the first in tokens.
  // Feature names and queries aren't necessarily index-aligned, so it's important
  // to "slide" the feature name along the query to look for matches.
  // For instance,
  // "Pennsylvania Ave NW, Washington, DC"
  // "1600 Pennsylvania Ave"
  // doesn't match at all, but
  //      "Pennsylvania Ave NW, Washington, DC"
  // "1600 Pennsylvania Ave"
  // is a partial match. Fuzzy matching helps match buildings
  // missing addresses in OSM, and it helps be more flexible in general.
  for (size_t offset = 0; offset < sliceCount + tokenCount; ++offset)
  {
    // Reset error and match-length count for each offset attempt.
    ErrorsMade totalErrorsMade(0);
    size_t matchedLength = 0;
    // Highest quality namescore possible for this offset
    NameScore nameScore = NameScore::SUBSTRING;
    // Prefix & full matches must test starting at the same index. (tokenIndex == i)
    if (offset + 1 == tokenCount)
    {
      if (sliceCount == tokenCount)
        nameScore = NameScore::FULL_MATCH;
      else
        nameScore = NameScore::FULL_PREFIX;
    }
    bool isAltOrOldName = false;
    // Iterate through the entire slice. Incomplete matches can still be good.
    // Using this slice & token as an example:
    //                              0   1   2   3   4   5   6
    // slice count=7:              foo bar baz bot bop bip bla
    // token count=3:      bar baz bot
    //
    // When offset = 0, tokenIndex should start at +2:
    //                 0   1   2   3   4   5   6
    // slice =         foo bar baz bot bop bip bla
    // token = baz bot bop
    //          0   1   2
    //
    // Offset must run to 8 to test all potential matches. (slice + token - 1)
    // Making tokenIndex start at -6 (-sliceSize)
    //                  0   1   2   3   4   5   6
    // slice =         foo bar baz bot bop bip bla
    // token =                                 baz bot bop
    //                 -6  -5  -4  -3  -2  -1   0   1   2
    for (size_t i = std::max(0, int(offset) + 1 - int(tokenCount));
                i < std::min(sliceCount, offset + 1); ++i)
    {
      size_t const tokenIndex = i + tokenCount - 1 - offset;

      // Count the errors. If GetErrorsMade finds a match, count it towards
      // the matched length and check against the prior best.
      auto errorsMade = impl::GetErrorsMade(slice.Get(i), tokens[tokenIndex]);

      // Also, check like prefix, if we've got token-like matching errors.
      if (!errorsMade.IsZero() && slice.IsPrefix(i))
      {
        auto const prefixErrors = impl::GetPrefixErrorsMade(slice.Get(i), tokens[tokenIndex]);
        if (prefixErrors.IsBetterThan(errorsMade))
        {
          // NAME_SCORE_PREFIX with less errors is better than NAME_SCORE_FULL_MATCH.
          /// @see RankingInfo_PrefixVsFull test.
          errorsMade = prefixErrors;
          if (nameScore == NameScore::FULL_MATCH || nameScore == NameScore::FULL_PREFIX)
            nameScore = NameScore::PREFIX;
        }
      }

      // This block was responsibe for: name = "X Y", query = "X Z" => score is FULL_PREFIX.
      // IMHO, this is very controversial, keep it as SUBSTRING.
      /*
      if (!errorsMade.IsValid() && nameScore == NameScore::FULL_MATCH && matchedLength)
      {
        nameScore = NameScore::FULL_PREFIX;
        errorsMade = ErrorsMade(0);
        // Don't count this token towards match length.
        matchedLength -= slice.Get(i).GetOriginal().size();
      }
      */

      if (errorsMade.IsValid())
      {
        // Update the match quality
        totalErrorsMade += errorsMade;
        matchedLength += slice.Get(i).GetOriginal().size();
        isAltOrOldName =
            lang == StringUtf8Multilang::kAltNameCode || lang == StringUtf8Multilang::kOldNameCode;
      }
      else
      {
        // If any token mismatches, this is at best a substring match.
        nameScore = NameScore::SUBSTRING;
      }
    }

    if (matchedLength == 0)
    {
      nameScore = NameScore::ZERO;
      totalErrorsMade = ErrorsMade();
    }
    scores.UpdateIfBetter(NameScores(nameScore, totalErrorsMade, isAltOrOldName, matchedLength));
  }

  // Uncomment for verbose search logging
  // LOG(LDEBUG, ("Match quality", search::DebugPrint(scores), "from", tokens, "into", slice));
  return scores;
}

template <typename Slice>
NameScores GetNameScores(std::string_view name, uint8_t lang, Slice const & slice)
{
  return GetNameScores(NormalizeAndTokenizeString(name), lang, slice);
}
}  // namespace search
