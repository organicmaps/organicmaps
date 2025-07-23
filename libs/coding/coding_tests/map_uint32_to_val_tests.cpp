#include "testing/testing.hpp"

#include "coding/map_uint32_to_val.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include <utility>
#include <vector>

namespace map_uint32_tests
{
using namespace std;

using BufferT = vector<uint8_t>;
using ValuesT = vector<uint32_t>;
using BuilderT = MapUint32ToValueBuilder<uint32_t>;
using MapT = MapUint32ToValue<uint32_t>;

UNIT_TEST(MapUint32Val_Small)
{
  {
    BuilderT builder;
    BufferT buffer;
    MemWriter writer(buffer);
    builder.Freeze(writer, [](Writer &, auto, auto) {});

    LOG(LINFO, ("Empty map size =", buffer.size()));

    MemReader reader(buffer.data(), buffer.size());
    auto map = MapT::Load(reader, [](NonOwningReaderSource &, uint32_t, ValuesT &) {});

    TEST_EQUAL(map->Count(), 0, ());
    uint32_t dummy;
    TEST(!map->Get(1, dummy), ());
  }

  {
    BuilderT builder;
    builder.Put(1, 777);
    BufferT buffer;
    MemWriter writer(buffer);
    builder.Freeze(writer, [](Writer & writer, auto b, auto e)
    {
      WriteVarUint(writer, *b++);
      TEST(b == e, ());
    });

    MemReader reader(buffer.data(), buffer.size());
    auto map = MapT::Load(reader, [](NonOwningReaderSource & source, uint32_t blockSize, ValuesT & values)
    {
      TEST_EQUAL(blockSize, 1, ("GetThreadsafe should pass optimal blockSize"));
      while (source.Size() > 0)
        values.push_back(ReadVarUint<uint32_t>(source));
      TEST_EQUAL(values.size(), 1, ());
    });

    TEST_EQUAL(map->Count(), 1, ());
    uint32_t val;
    TEST(map->GetThreadsafe(1, val), ());
    TEST_EQUAL(val, 777, ());
  }
}

UNIT_TEST(MapUint32Val_Smoke)
{
  vector<pair<uint32_t, uint32_t>> data;
  size_t const dataSize = 227;
  data.resize(dataSize);
  for (size_t i = 0; i < data.size(); ++i)
    data[i] = make_pair(static_cast<uint32_t>(i), static_cast<uint32_t>(i));

  BufferT buffer;
  {
    BuilderT builder;
    for (auto const & d : data)
      builder.Put(d.first, d.second);

    MemWriter writer(buffer);
    builder.Freeze(writer, [](Writer & w, BuilderT::Iter begin, BuilderT::Iter end)
    {
      for (auto it = begin; it != end; ++it)
        WriteToSink(w, *it);
    });
  }

  {
    MemReader reader(buffer.data(), buffer.size());
    auto table = MapUint32ToValue<uint32_t>::Load(
        reader, [](NonOwningReaderSource & source, uint32_t blockSize, ValuesT & values)
    {
      values.reserve(blockSize);
      while (source.Size() > 0)
        values.push_back(ReadPrimitiveFromSource<uint32_t>(source));
    });
    TEST(table.get(), ());

    for (auto const & d : data)
    {
      uint32_t res;
      TEST(table->Get(d.first, res), ());
      TEST_EQUAL(res, d.second, ());
      TEST(table->GetThreadsafe(d.first, res), ());
      TEST_EQUAL(res, d.second, ());
    }
  }
}
}  // namespace map_uint32_tests
