#include "testing/testing.hpp"

#include "base/macros.hpp"

#include <cstring>
#include <string>
#include <vector>

#include "3party/bsdiff-courgette/bsdiff/bsdiff_search.h"
#include "3party/bsdiff-courgette/divsufsort/divsufsort.h"

using namespace std;

// Adapted from 3party/bsdiff-courgette.
UNIT_TEST(BSDiffSearchTest_Search)
{
  // Initialize main string and the suffix array.
  // Positions:      000000000011111111111222222222333333333344444
  //                 012345678901234567890123456789012345678901234
  string const str = "the quick brown fox jumps over the lazy dog.";
  int const size = static_cast<int>(str.size());
  auto buf = reinterpret_cast<unsigned char const * const>(str.data());
  vector<divsuf::saidx_t> suffix_array(size + 1);
  divsuf::divsufsort_include_empty(buf, suffix_array.data(), size);

  // Specific queries.
  struct
  {
    int m_expMatchPos;  // -1 means "don't care".
    int m_expMatchSize;
    string m_query_str;
  } const testCases[] = {
      // Entire string: exact and unique.
      {0, 44, "the quick brown fox jumps over the lazy dog."},
      // Empty string: exact and non-unique.
      {-1, 0, ""},
      // Exact and unique suffix matches.
      {43, 1, "."},
      {31, 13, "the lazy dog."},
      // Exact and unique non-suffix matches.
      {4, 5, "quick"},
      {0, 9, "the quick"},  // Unique prefix.
      // Partial and unique matches.
      {16, 10, "fox jumps with the hosps"},  // Unique prefix.
      {18, 1, "xyz"},
      // Exact and non-unique match: take lexicographical first.
      {-1, 3, "the"},  // Non-unique prefix.
      {-1, 1, " "},
      // Partial and non-unique match: no guarantees on |match.pos|!
      {-1, 4, "the apple"},  // query      < "the l"... < "the q"...
      {-1, 4, "the opera"},  // "the l"... < query      < "the q"...
      {-1, 4, "the zebra"},  // "the l"... < "the q"... < query
      // Prefix match dominates suffix match (unique).
      {26, 5, "over quick brown fox"},
      // Empty matchs.
      {-1, 0, ","},
      {-1, 0, "1234"},
      {-1, 0, "THE QUICK BROWN FOX"},
      {-1, 0, "(the"},
  };

  for (size_t idx = 0; idx < ARRAY_SIZE(testCases); ++idx)
  {
    auto const & testCase = testCases[idx];
    int const querySize = static_cast<int>(testCase.m_query_str.size());
    auto query_buf = reinterpret_cast<unsigned char const * const>(testCase.m_query_str.data());

    // Perform the search.
    bsdiff::SearchResult const match =
        bsdiff::search<decltype(suffix_array)>(suffix_array, buf, size, query_buf, querySize);

    // Check basic properties and match with expected values.
    TEST_GREATER_OR_EQUAL(match.size, 0, ());
    TEST_LESS_OR_EQUAL(match.size, querySize, ());
    if (match.size > 0)
    {
      TEST_GREATER_OR_EQUAL(match.pos, 0, ());
      TEST_LESS_OR_EQUAL(match.pos, size - match.size, ());
      TEST_EQUAL(0, memcmp(buf + match.pos, query_buf, match.size), ());
    }
    if (testCase.m_expMatchPos >= 0)
    {
      TEST_EQUAL(testCase.m_expMatchPos, match.pos, ());
    }
    TEST_EQUAL(testCase.m_expMatchSize, match.size, ());
  }
}

// Adapted from 3party/bsdiff-courgette.
UNIT_TEST(BSDiffSearchTest_SearchExact)
{
  string const testCases[] = {
      "a",
      "aa",
      "az",
      "za",
      "aaaaa",
      "CACAO",
      "banana",
      "tobeornottobe",
      "the quick brown fox jumps over the lazy dog.",
      "elephantelephantelephantelephantelephant",
      "011010011001011010010110011010010",
  };
  for (size_t idx = 0; idx < ARRAY_SIZE(testCases); ++idx)
  {
    int const size = static_cast<int>(testCases[idx].size());
    unsigned char const * const buf =
        reinterpret_cast<unsigned char const * const>(testCases[idx].data());

    vector<divsuf::saidx_t> suffix_array(size + 1);
    divsuf::divsufsort_include_empty(buf, suffix_array.data(), size);

    // Test exact matches for every non-empty substring.
    for (int lo = 0; lo < size; ++lo)
    {
      for (int hi = lo + 1; hi <= size; ++hi)
      {
        string query(buf + lo, buf + hi);
        int querySize = static_cast<int>(query.size());
        CHECK_EQUAL(querySize, hi - lo, ());
        unsigned char const * const query_buf =
            reinterpret_cast<unsigned char const * const>(query.c_str());
        bsdiff::SearchResult const match =
            bsdiff::search<decltype(suffix_array)>(suffix_array, buf, size, query_buf, querySize);

        TEST_EQUAL(querySize, match.size, ());
        TEST_GREATER_OR_EQUAL(match.pos, 0, ());
        TEST_LESS_OR_EQUAL(match.pos, size - match.size, ());
        string const suffix(buf + match.pos, buf + size);
        TEST_EQUAL(suffix.substr(0, querySize), query, ());
      }
    }
  }
}
