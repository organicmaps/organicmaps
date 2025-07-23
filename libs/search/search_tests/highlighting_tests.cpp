#include "testing/testing.hpp"

#include "search/highlighting.hpp"

#include "indexer/feature_covering.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <cstdarg>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace
{
using TestResult = pair<uint16_t, uint16_t>;
using TokensVector = vector<strings::UniString>;
using TestResultVector = vector<TestResult>;

struct TestData
{
  string m_input;
  TokensVector m_lowTokens;
  TestResultVector m_results;

  TestData(char const * inp, char const ** lToks, size_t lowTokCount, size_t resCount, ...)
  {
    m_input = inp;
    for (size_t i = 0; i < lowTokCount; ++i)
      m_lowTokens.push_back(strings::MakeUniString(lToks[i]));

    va_list ap;
    va_start(ap, resCount);
    for (size_t i = 0; i < resCount; ++i)
    {
      uint16_t const pos = va_arg(ap, int);
      uint16_t const len = va_arg(ap, int);
      AddResult(pos, len);
    }
    va_end(ap);
  }

  void AddResult(uint16_t pos, uint16_t len) { m_results.emplace_back(pos, len); }
};

class CheckRange
{
  size_t m_idx;
  TestResultVector const & m_results;

public:
  explicit CheckRange(TestResultVector const & results) : m_idx(0), m_results(results) {}

  ~CheckRange() { TEST_EQUAL(m_idx, m_results.size(), ()); }

  void operator()(pair<uint16_t, uint16_t> const & range)
  {
    ASSERT(m_idx < m_results.size(), ());
    TEST_EQUAL(range, m_results[m_idx], ());
    ++m_idx;
  }
};
}  // namespace

UNIT_TEST(SearchStringTokensIntersectionRange)
{
  char const * str0 = "улица Карла Маркса";
  char const * str1 = "ул. Карла Маркса";
  char const * str2 = "Карлов Мост";
  char const * str3 = "цирк";
  char const * str4 = "Беларусь Минск Цирк";
  char const * str5 = "Беларусь, Цирк, Минск";
  char const * str6 = "";
  char const * str7 = ".-карла маркса ";
  char const * str8 = ".-карла, маркса";
  char const * str9 = "улица Карла Либнехта";
  char const * str10 = "улица Карбышева";
  char const * str11 = "улица Максима Богдановича";

  char const * lowTokens0[] = {"карла", "маркса"};
  char const * lowTokens1[] = {"цирк", "минск"};
  char const * lowTokens2[] = {"ар"};
  char const * lowTokens3[] = {"карла", "м"};
  char const * lowTokens4[] = {"кар", "мар"};
  char const * lowTokens5[] = {"минск", "цирк"};
  char const * lowTokens6[] = {""};
  char const * lowTokens7[] = {"цирк", ""};
  char const * lowTokens8[] = {"ул", "кар"};
  char const * lowTokens9[] = {"ул", "бог"};

  vector<TestData> tests;
  // fill test data
  tests.push_back(TestData(str0, lowTokens0, 2, 2, 6, 5, 12, 6));
  tests.push_back(TestData(str1, lowTokens0, 2, 2, 4, 5, 10, 6));
  tests.push_back(TestData(str2, lowTokens0, 2, 0));
  tests.push_back(TestData(str10, lowTokens8, 2, 2, 0, 2, 6, 3));
  tests.push_back(TestData(str4, lowTokens1, 2, 2, 9, 5, 15, 4));

  tests.push_back(TestData(str0, lowTokens2, 1, 0));
  tests.push_back(TestData(str2, lowTokens2, 1, 0));
  tests.push_back(TestData(str0, lowTokens3, 2, 2, 6, 5, 12, 1));
  tests.push_back(TestData(str1, lowTokens3, 2, 2, 4, 5, 10, 1));
  tests.push_back(TestData(str0, lowTokens4, 2, 2, 6, 3, 12, 3));

  tests.push_back(TestData(str3, lowTokens1, 2, 1, 0, 4));
  tests.push_back(TestData(str5, lowTokens5, 2, 2, 10, 4, 16, 5));
  tests.push_back(TestData(str6, lowTokens6, 1, 0));
  tests.push_back(TestData(str6, lowTokens7, 2, 0));
  tests.push_back(TestData(str5, lowTokens7, 2, 1, 10, 4));

  tests.push_back(TestData(str8, lowTokens3, 2, 2, 2, 5, 9, 1));
  tests.push_back(TestData(str7, lowTokens0, 2, 2, 2, 5, 8, 6));
  tests.push_back(TestData(str0, lowTokens8, 2, 2, 0, 2, 6, 3));
  tests.push_back(TestData(str9, lowTokens8, 2, 2, 0, 2, 6, 3));
  tests.push_back(TestData(str11, lowTokens9, 2, 2, 0, 2, 14, 3));

  for (TestData const & data : tests)
  {
    search::SearchStringTokensIntersectionRanges(data.m_input, data.m_lowTokens.begin(), data.m_lowTokens.end(),
                                                 CheckRange(data.m_results));
  }
}
