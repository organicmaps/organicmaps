#include "testing/testing.hpp"

#include "indexer/classificator.hpp"

#include "generator/generator_tests_support/test_with_classificator.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

using namespace generator::tests_support;
using namespace std;

UNIT_CLASS_TEST(TestWithClassificator, Classificator_GetType)
{
  Classificator const & c = classif();

  uint32_t const type1 = c.GetTypeByPath({"natural", "coastline"});
  TEST_NOT_EQUAL(0, type1, ());
  TEST(c.IsTypeValid(type1), ());
  TEST_EQUAL(type1, c.GetTypeByReadableObjectName("natural-coastline"), ());

  uint32_t const type2 = c.GetTypeByPath({"amenity", "parking", "private"});
  TEST_NOT_EQUAL(0, type2, ());
  TEST(c.IsTypeValid(type2), ());
  TEST_EQUAL(type2, c.GetTypeByReadableObjectName("amenity-parking-private"), ());

  TEST_EQUAL(0, c.GetTypeByPathSafe({"nonexisting", "type"}), ());
  TEST_EQUAL(0, c.GetTypeByReadableObjectName("nonexisting-type"), ());
  TEST(!c.IsTypeValid(0), ());
}

UNIT_CLASS_TEST(TestWithClassificator, Classificator_CoastlineType)
{
  Classificator const & c = classif();

  uint32_t const type = c.GetTypeByPath({"natural", "coastline"});
  TEST(c.IsTypeValid(type), ());
  TEST_EQUAL(type, c.GetCoastType(), ());
}

UNIT_CLASS_TEST(TestWithClassificator, Classificator_GetIndex)
{
  Classificator const & c = classif();

  uint32_t const type = c.GetTypeByPath({"railway", "station", "subway"});
  uint32_t const index = c.GetIndexForType(type);
  TEST(c.IsTypeValid(type), ());
  TEST_EQUAL(type, c.GetTypeForIndex(index), ());
}

UNIT_CLASS_TEST(TestWithClassificator, Classificator_Subtree)
{
  Classificator const & c = classif();

  uint32_t const cityType = c.GetTypeByPath({"place", "city"});

  vector<vector<string>> const expectedPaths = {
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

  vector<uint32_t> subtreeTypes;
  c.ForEachInSubtree([&subtreeTypes](uint32_t type) { subtreeTypes.push_back(type); }, cityType);
  sort(subtreeTypes.begin(), subtreeTypes.end());

  TEST_EQUAL(expectedTypes, subtreeTypes, ());
}

UNIT_CLASS_TEST(TestWithClassificator, Classificator_StableIndex)
{
  // mapcss-mapping.csv:
  //
  // amenity|parking|underground|fee;[amenity=parking][location=underground][fee?],[amenity=parking][parking=underground][fee?];x;name;int_name;356;amenity|parking|underground
  // amenity|parking|underground;[amenity=parking][location=underground],[amenity=parking][parking=underground];;name;int_name;357;
  //
  // The main definition of amenity|parking|underground goes second but we must use it for index. Test both indexes.
  // belong to amenity|parking|underground type and GetIndexForType returns second  one.

  Classificator const & c = classif();

  uint32_t const type = c.GetTypeByPath({"amenity", "parking", "underground"});
  TEST(c.IsTypeValid(type), ());
  uint32_t const index = c.GetIndexForType(type);
  TEST_EQUAL(index, 357 - 1, ());
  TEST_NOT_EQUAL(type, c.GetTypeForIndex(356 - 1), ());  // Restored underground-fee
  TEST_EQUAL(type, c.GetTypeForIndex(357 - 1), ());
}
