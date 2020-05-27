#include "testing/testing.hpp"

#include "base/id_generator.hpp"

#include <limits>
#include <string>
#include <vector>

UNIT_TEST(IdGenerator_Smoke)
{
  auto const initial = base::IdGenerator<uint8_t>::GetInitialId();
  TEST_EQUAL(initial, "1", ());

  auto constexpr kOverflowCount = 5;
  auto constexpr kItemsCount = std::numeric_limits<uint8_t>::max();

  auto id = initial;
  auto i = 2;
  for (auto k = 0; k < kOverflowCount; ++k)
  {
    for (; i <= kItemsCount; ++i)
    {
      std::string target;
      for (auto j = 0; j < k; ++j)
        target += std::to_string(kItemsCount);

      target += std::to_string(i);
      id = base::IdGenerator<uint8_t>::GetNextId(id);
      TEST_EQUAL(target, id, ());
    }
    i = 1;
  }
}
