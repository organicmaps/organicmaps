#include "testing/testing.hpp"

#include "coding/map_uint32_to_val.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <cstdint>
#include <utility>
#include <vector>

using namespace std;

using Buffer = vector<uint8_t>;

UNIT_TEST(MapUint32ValTest)
{
  vector<pair<uint32_t, uint32_t>> data;
  size_t const dataSize = 227;
  data.resize(dataSize);
  for (size_t i = 0; i < data.size(); ++i)
    data[i] = make_pair(static_cast<uint32_t>(i), static_cast<uint32_t>(i));

  Buffer buffer;
  {
    MapUint32ToValueBuilder<uint32_t> builder;

    for (auto const & d : data)
      builder.Put(d.first, d.second);

    MemWriter<Buffer> writer(buffer);

    auto const trivialWriteBlockCallback = [](Writer & w, vector<uint32_t>::const_iterator begin,
                                              vector<uint32_t>::const_iterator end) {
      for (auto it = begin; it != end; ++it)
        WriteToSink(w, *it);
    };
    builder.Freeze(writer, trivialWriteBlockCallback);
  }

  {
    MemReader reader(buffer.data(), buffer.size());
    auto const trivialReadBlockCallback = [](NonOwningReaderSource & source, uint32_t blockSize,
                                             vector<uint32_t> & values) {
      values.resize(blockSize);
      for (size_t i = 0; i < blockSize && source.Size() > 0; ++i)
      {
        values[i] = ReadPrimitiveFromSource<uint32_t>(source);
      }
    };
    auto table = MapUint32ToValue<uint32_t>::Load(reader, trivialReadBlockCallback);
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
