#include "testing/testing.hpp"

#include "base/collection_cast.hpp"

#include <list>
#include <vector>

UNIT_TEST(collection_cast)
{
  TEST_EQUAL((std::list<int>{
                 1,
                 2,
                 3,
                 4,
             }),
             base::collection_cast<std::list>(std::vector<int>{1, 2, 3, 4}), ());
}
