#include "testing/testing.hpp"

#include "drape/utils/vertex_decl.hpp"

#include "base/buffer_vector.hpp"

#include <boost/container/small_vector.hpp>

#include <random>
#include <vector>

namespace vertex_buffer_tests
{
#ifndef DEBUG
std::vector<uint32_t> GenerateSizes(uint32_t min, uint32_t max)
{
  std::vector<uint32_t> res;

  std::uniform_int_distribution<uint64_t> randDist(min, max);
  std::random_device randDevice;
  std::mt19937 randEngine(randDevice());

  for (size_t i = 0; i < 1000000; ++i)
    res.push_back(randDist(randEngine));

  return res;
}

UNIT_TEST(VertexBuffer_Benchmark)
{
  using ValueT = gpu::TextOutlinedStaticVertex;
  // using ValueT = gpu::TextDynamicVertex
  ValueT objs[] = {
      {{1, 1}, {2, 2}, {3, 3}},
      {{4, 4}, {5, 5}, {6, 6}},
      {{7, 7}, {8, 8}, {9, 9}},
  };

  // Very comfortable for buffer_vector<ValueT, kUpperBound>.
  uint32_t constexpr kUpperBound = 128;
  std::vector<uint32_t> sizes = GenerateSizes(16, kUpperBound);

  uint64_t t1, t2, t3, t4;
  {
    base::Timer timer;
    for (uint32_t sz : sizes)
    {
      std::vector<ValueT> v;
      for (size_t i = 0; i < sz; ++i)
        v.push_back(objs[i % std::size(objs)]);
    }
    t1 = timer.ElapsedMilliseconds();
  }

  {
    base::Timer timer;
    for (uint32_t sz : sizes)
    {
      buffer_vector<ValueT, kUpperBound> v;
      for (size_t i = 0; i < sz; ++i)
        v.push_back(objs[i % std::size(objs)]);
    }
    t2 = timer.ElapsedMilliseconds();
  }

  {
    base::Timer timer;
    for (uint32_t sz : sizes)
    {
      boost::container::small_vector<ValueT, kUpperBound> v;
      for (size_t i = 0; i < sz; ++i)
        v.push_back(objs[i % std::size(objs)]);
    }
    t3 = timer.ElapsedMilliseconds();
  }

  {
    base::Timer timer;
    for (uint32_t sz : sizes)
    {
      std::vector<ValueT> v;
      v.reserve(sz);
      for (size_t i = 0; i < sz; ++i)
        v.push_back(objs[i % std::size(objs)]);
    }
    t4 = timer.ElapsedMilliseconds();
  }

  LOG(LINFO,
      ("vector time:", t1, "buffer_vector time:", t2, "boost::small_vector time:", t3, "reserved vector time:", t4));
  TEST_LESS(t2, t1, ());
  TEST_LESS(t3, t2, ());
  // TODO: Fix this condition
  // TEST_LESS(t4, t3, ());
}
#endif
}  // namespace vertex_buffer_tests
