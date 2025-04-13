#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_with_classificator.hpp"

#include "indexer/classificator.hpp"

#include "search/localities_source.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

using namespace std;
using generator::tests_support::TestWithClassificator;

UNIT_CLASS_TEST(TestWithClassificator, Smoke)
{
  search::LocalitiesSource ls;

  vector<vector<string>> const expectedPaths = {
      {"place", "town"},
      {"place", "city"},
      {"place", "city", "capital"},
      {"place", "city", "capital", "2"},
      {"place", "city", "capital", "3"},
      {"place", "city", "capital", "4"},
      {"place", "city", "capital", "5"},
      {"place", "city", "capital", "6"},
      {"place", "city", "capital", "7"},
      {"place", "city", "capital", "8"},
      {"place", "city", "capital", "9"},
      {"place", "city", "capital", "10"},
      {"place", "city", "capital", "11"},
  };

  vector<uint32_t> expectedTypes;
  for (auto const & path : expectedPaths)
    expectedTypes.push_back(classif().GetTypeByPath(path));
  sort(expectedTypes.begin(), expectedTypes.end());

  vector<uint32_t> localitiesSourceTypes;
  ls.ForEachType([&localitiesSourceTypes](uint32_t type) { localitiesSourceTypes.push_back(type); });
  sort(localitiesSourceTypes.begin(), localitiesSourceTypes.end());

  TEST_EQUAL(expectedTypes, localitiesSourceTypes, ());
}
