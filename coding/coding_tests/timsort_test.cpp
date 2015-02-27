#include "../../testing/testing.hpp"

#include "../timsort/timsort.hpp"

#include "../../std/algorithm.hpp"
#include "../../std/vector.hpp"

int intCmp(void const * plhs, void const * prhs)
{
  int const lhs = *static_cast<int const *>(plhs);
  int const rhs = *static_cast<int const *>(prhs);
  if (lhs < rhs)
    return -1;
  if (lhs == rhs)
    return 0;
  return 1;
}

UNIT_TEST(TimSort_Empty) {
  vector<int> buffer;
  timsort(&buffer[0], 0, sizeof(int), intCmp);
}

UNIT_TEST(Timsort_Simple) {
  vector<int> buffer = { 5, 1, 0, 7, 9, 10, 1, 3, 4 };

  vector<int> sorted_buffer(buffer);
  sort(sorted_buffer.begin(), sorted_buffer.end());

  timsort(&buffer[0], buffer.size(), sizeof(int), intCmp);

  TEST_EQUAL(sorted_buffer, buffer, ());
}
