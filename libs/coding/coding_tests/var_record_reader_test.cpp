#include "testing/testing.hpp"

#include "coding/reader.hpp"
#include "coding/var_record_reader.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "base/macros.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace
{
struct SaveForEachParams
{
  explicit SaveForEachParams(vector<pair<uint64_t, string>> & data) : m_data(data) {}

  void operator()(uint64_t pos, vector<uint8_t> && data) const
  {
    m_data.emplace_back(pos, string(data.begin(), data.end()));
  }

  vector<pair<uint64_t, string>> & m_data;
};

}  // namespace

UNIT_TEST(VarRecordReader_Simple)
{
  vector<uint8_t> data;
  char const longString[] =
      "0123456789012345678901234567890123456789012345678901234567890123456789"
      "012345678901234567890123456789012345678901234567890123456789012345";
  size_t const longStringSize = sizeof(longString) - 1;
  TEST_GREATER(longStringSize, 128, ());
  {
    MemWriter<vector<uint8_t>> writer(data);
    WriteVarUint(writer, 3U);                  //  0
    writer.Write("abc", 3);                    //  1
    WriteVarUint(writer, longStringSize);      //  4
    writer.Write(longString, longStringSize);  //  6
    WriteVarUint(writer, 4U);                  //  6 + longStringSize
    writer.Write("defg", 4);                   //  7 + longStringSize
                                               // 11 + longStringSize
  }

  MemReader reader(&data[0], data.size());
  VarRecordReader<MemReader> recordReader(reader);

  auto r = recordReader.ReadRecord(0);
  TEST_EQUAL(string(r.begin(), r.end()), "abc", ());

  r = recordReader.ReadRecord(6 + longStringSize);
  TEST_EQUAL(string(r.begin(), r.end()), "defg", ());

  r = recordReader.ReadRecord(4);
  TEST_EQUAL(string(r.begin(), r.end()), longString, ());

  vector<pair<uint64_t, string>> forEachCalls;
  recordReader.ForEachRecord(SaveForEachParams(forEachCalls));
  vector<pair<uint64_t, string>> expectedForEachCalls = {{0, "abc"}, {4, longString}, {6 + longStringSize, "defg"}};
  TEST_EQUAL(forEachCalls, expectedForEachCalls, ());
}
