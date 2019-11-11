#include "testing/testing.hpp"

#include "indexer/complex/serdes_utils.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <algorithm>
#include <cstdint>
#include <list>
#include <set>
#include <vector>

namespace
{
using ByteVector = std::vector<uint8_t>;

template <typename Cont>
void CollectionPrimitiveTest(Cont const & cont)
{
  ByteVector buffer;
  MemWriter<decltype(buffer)> writer(buffer);
  WriterSink<decltype(writer)> sink(writer);
  coding_utils::WriteCollectionPrimitive(sink, cont);

  MemReader reader(buffer.data(), buffer.size());
  ReaderSource<decltype(reader)> src(reader);
  Cont res;
  coding_utils::ReadCollectionPrimitive(src, std::inserter(res, std::end(res)));
  TEST_EQUAL(cont, res, ());
}

UNIT_TEST(Utils_CollectionPrimitive)
{
  {
    std::list<uint8_t> const cont{1, 2, 3, 4};
    CollectionPrimitiveTest(cont);
  }
  {
    std::list<int8_t> const cont{-1, -2, -3, -4};
    CollectionPrimitiveTest(cont);
  }
  {
    std::vector<uint32_t> const cont{1, 2, 3, 4};
    CollectionPrimitiveTest(cont);
  }
  {
    std::vector<int32_t> const cont{-1, -2, -3, -4};
    CollectionPrimitiveTest(cont);
  }
  {
    std::set<uint64_t> const cont{1, 2, 3, 4};
    CollectionPrimitiveTest(cont);
  }
  {
    std::set<int64_t> const cont{-1, -2, -3, -4};
    CollectionPrimitiveTest(cont);
  }
}
}  // namespace
