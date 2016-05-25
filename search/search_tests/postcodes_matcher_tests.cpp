#include "../../testing/testing.hpp"

#include "search/query_params.hpp"
#include "search/v2/postcodes_matcher.hpp"
#include "search/v2/token_slice.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

using namespace strings;

namespace search
{
namespace v2
{
namespace
{
bool LooksLikePostcode(string const & s, bool checkPrefix)
{
  vector<UniString> tokens;
  bool const lastTokenIsPrefix =
      TokenizeStringAndCheckIfLastTokenIsPrefix(s, tokens, search::Delimiters());

  size_t const numTokens = tokens.size();

  QueryParams params;
  if (checkPrefix && lastTokenIsPrefix)
  {
    params.m_prefixTokens.push_back(tokens.back());
    tokens.pop_back();
  }

  for (auto const & token : tokens)
  {
    params.m_tokens.emplace_back();
    params.m_tokens.back().push_back(token);
  }

  return LooksLikePostcode(TokenSlice(params, 0, numTokens));
}

UNIT_TEST(PostcodesMatcher_Smoke)
{
  TEST(LooksLikePostcode("141701", false /* checkPrefix */), ());
  TEST(LooksLikePostcode("141", true /* checkPrefix */), ());
  TEST(LooksLikePostcode("BA6 8JP", true /* checkPrefix */), ());
  TEST(LooksLikePostcode("BA6 8JP", true /* checkPrefix */), ());
  TEST(LooksLikePostcode("BA22 9HR", true /* checkPrefix */), ());
  TEST(LooksLikePostcode("BA22", true /* checkPrefix */), ());
  TEST(LooksLikePostcode("DE56 4FW", true /* checkPrefix */), ());
  TEST(LooksLikePostcode("NY 1000", true /* checkPrefix */), ());
  TEST(LooksLikePostcode("AZ 85203", true /* checkPrefix */), ());
  TEST(LooksLikePostcode("AZ", true /* checkPrefix */), ());

  TEST(LooksLikePostcode("803 0271", true /* checkPrefix */), ());
  TEST(LooksLikePostcode("803-0271", true /* checkPrefix */), ());
  TEST(LooksLikePostcode("〒803-0271", true /* checkPrefix */), ());

  TEST(!LooksLikePostcode("1 мая", true /* checkPrefix */), ());
  TEST(!LooksLikePostcode("1 мая улица", true /* checkPrefix */), ());
  TEST(!LooksLikePostcode("москва", true /* checkPrefix */), ());
  TEST(!LooksLikePostcode("39 с 79", true /* checkPrefix */), ());
}
}  // namespace
}  // namespace v2
}  // namespace search
