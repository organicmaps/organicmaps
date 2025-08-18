#include "testing/testing.hpp"

#include "coding/sparse_vector.hpp"

UNIT_TEST(SparseVector_Smoke)
{
  uint32_t const arr[] = {0, 0, 5, 0, 7, 1000, 0, 0, 1, 0};
  uint64_t const count = std::size(arr);

  coding::SparseVectorBuilder<uint32_t> builder(count);
  for (uint32_t v : arr)
    if (v == 0)
      builder.PushEmpty();
    else
      builder.PushValue(v);

  auto vec = builder.Build();

  TEST_EQUAL(vec.GetSize(), count, ());
  for (size_t i = 0; i < count; ++i)
  {
    TEST_EQUAL(vec.Has(i), (arr[i] != 0), ());
    if (arr[i] != 0)
      TEST_EQUAL(vec.Get(i), arr[i], ());
  }
}
