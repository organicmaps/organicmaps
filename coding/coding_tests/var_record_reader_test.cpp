#include "testing/testing.hpp"
#include "coding/var_record_reader.hpp"
#include "coding/varint.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"
#include "base/macros.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace
{
  struct SaveForEachParams
  {
    explicit SaveForEachParams(vector<pair<uint64_t, string> > & data) : m_Data(data) {}
    void operator () (uint64_t pos, char const * pData, uint32_t size) const
    {
      m_Data.push_back(make_pair(pos, string(pData, pData + size)));
    }
    vector<pair<uint64_t, string> > & m_Data;
  };

}

UNIT_TEST(VarRecordReader_Simple)
{
  vector<char> data;
  char const longString[] = "0123456789012345678901234567890123456789012345678901234567890123456789"
                            "012345678901234567890123456789012345678901234567890123456789012345";
  size_t const longStringSize = sizeof(longString) - 1;
  TEST_GREATER(longStringSize, 128, ());
  {
    MemWriter<vector<char> > writer(data);
    WriteVarUint(writer, 3U);                   //  0
    writer.Write("abc", 3);                     //  1
    WriteVarUint(writer, longStringSize);       //  4
    writer.Write(longString, longStringSize);   //  6
    WriteVarUint(writer, 4U);                   //  6 + longStringSize
    writer.Write("defg", 4);                    //  7 + longStringSize
                                                // 11 + longStringSize
  }

  uint32_t chunkSizes[] = {4, 5, 63, 64, 65, 1000};
  for (uint32_t chunkSize = 0; chunkSize < ARRAY_SIZE(chunkSizes); ++chunkSize)
  {
    MemReader reader(&data[0], data.size());
    VarRecordReader<MemReader, &VarRecordSizeReaderVarint> recordReader(
        reader, chunkSizes[chunkSize]);

    vector<char> r;
    uint32_t offset, size;

    TEST_EQUAL(4, recordReader.ReadRecord(0, r, offset, size), ());
    r.resize(size);
    TEST_EQUAL(string(r.begin() + offset, r.end()), "abc", ());

    TEST_EQUAL(11 + longStringSize, recordReader.ReadRecord(6 + longStringSize, r, offset, size), ());
    r.resize(size);
    TEST_EQUAL(string(r.begin() + offset, r.end()), "defg", ());

    TEST_EQUAL(6 + longStringSize, recordReader.ReadRecord(4, r, offset, size), ());
    r.resize(size);
    TEST_EQUAL(string(r.begin() + offset, r.end()), longString, ());

    vector<pair<uint64_t, string> > forEachCalls;
    recordReader.ForEachRecord(SaveForEachParams(forEachCalls));
    vector<pair<uint64_t, string> > expectedForEachCalls;
    expectedForEachCalls.push_back(pair<uint64_t, string>(0, "abc"));
    expectedForEachCalls.push_back(pair<uint64_t, string>(4, longString));
    expectedForEachCalls.push_back(pair<uint64_t, string>(6 + longStringSize, "defg"));
    TEST_EQUAL(forEachCalls, expectedForEachCalls, ());
  }
}
