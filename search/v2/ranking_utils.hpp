#pragma once

#include "search/v2/geocoder.hpp"
#include "search/v2/search_model.hpp"

#include "base/string_utils.hpp"

#include "std/cstdint.hpp"
#include "std/limits.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace search
{
struct SearchQueryParams;

namespace v2
{
// The order and numeric values are important here.  Please, check all
// use-cases before changing this enum.
enum NameScore
{
  NAME_SCORE_ZERO = 0,
  NAME_SCORE_SUBSTRING_PREFIX = 1,
  NAME_SCORE_SUBSTRING = 2,
  NAME_SCORE_FULL_MATCH_PREFIX = 3,
  NAME_SCORE_FULL_MATCH = 4,

  NAME_SCORE_COUNT
};

NameScore GetNameScore(string const & name, SearchQueryParams const & params, size_t startToken,
                       size_t endToken);

NameScore GetNameScore(vector<strings::UniString> const & tokens, SearchQueryParams const & params,
                       size_t startToken, size_t endToken);

string DebugPrint(NameScore score);
}  // namespace v2
}  // namespace search
