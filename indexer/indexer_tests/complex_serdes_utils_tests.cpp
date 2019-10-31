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

template <typename T, typename ValueType = typename T::value_type>
void DeltaEncodeDecodelTest(T const & cont, ValueType base = {})
{
  ByteVector buffer;
  MemWriter<decltype(buffer)> writer(buffer);
  WriterSink<decltype(writer)> sink(writer);
  coding_utils::DeltaEncode<ValueType>(sink, cont, base);

  MemReader reader(buffer.data(), buffer.size());
  ReaderSource<decltype(reader)> src(reader);
  T res;
  coding_utils::DeltaDecode<ValueType>(src, std::inserter(res, std::end(res)), base);
  TEST_EQUAL(cont, res, ());
}

template <typename T>
void CollectionPrimitiveTest(T const & cont)
{
  ByteVector buffer;
  MemWriter<decltype(buffer)> writer(buffer);
  WriterSink<decltype(writer)> sink(writer);
  coding_utils::WriteCollectionPrimitive(sink, cont);

  MemReader reader(buffer.data(), buffer.size());
  ReaderSource<decltype(reader)> src(reader);
  T res;
  coding_utils::ReadCollectionPrimitive(src, std::inserter(res, std::end(res)));
  TEST_EQUAL(cont, res, ());
}

UNIT_TEST(Utils_DeltaEncodeDecode)
{
  {
    std::vector<uint32_t> cont;
    DeltaEncodeDecodelTest(cont);
  }
  {
    std::list<uint32_t> cont{1, 2, 3, 4};
    cont.sort();
    DeltaEncodeDecodelTest(cont);
  }
  {
    std::list<uint32_t> cont{6, 7, 30, 40};
    cont.sort();
    DeltaEncodeDecodelTest(cont, 6);
  }
  {
    std::list<int32_t> cont{-1, -2, -3, -4};
    cont.sort();
    DeltaEncodeDecodelTest(cont);
  }
  {
    std::vector<uint32_t> cont{1, 2, 3, 4, 32, 124};
    std::sort(std::begin(cont), std::end(cont));
    DeltaEncodeDecodelTest(cont, 1);
  }
  {
    std::vector<int32_t> cont{-1, -2, -3, -4, 23, 67};
    std::sort(std::begin(cont), std::end(cont));
    DeltaEncodeDecodelTest(cont);
  }
  {
    std::set<uint64_t> cont{1, 2, 3, 4, 999, 100124, 243};
    DeltaEncodeDecodelTest(cont);
  }
  {
    std::set<int64_t> cont{-1, -2, -3, -4};
    DeltaEncodeDecodelTest(cont);
  }
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
