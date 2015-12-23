#include "testing/testing.hpp"

#include "base/collection_cast.hpp"

#include "std/list.hpp"
#include "std/vector.hpp"

UNIT_TEST(collection_cast)
{
  TEST_EQUAL((list<int>{1, 2, 3, 4, }),  my::collection_cast<list>(vector<int> {1, 2, 3, 4}), ());
}
