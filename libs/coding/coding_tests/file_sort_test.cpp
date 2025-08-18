#include "testing/testing.hpp"

#include "coding/file_sort.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

using namespace std;

namespace
{
void TestFileSorter(vector<uint32_t> & data, char const * tmpFileName, size_t bufferSize)
{
  vector<char> serial;
  typedef MemWriter<vector<char>> MemWriterType;
  MemWriterType writer(serial);
  typedef WriterFunctor<MemWriterType> OutT;
  OutT out(writer);
  FileSorter<uint32_t, OutT> sorter(bufferSize, tmpFileName, out);
  for (size_t i = 0; i < data.size(); ++i)
    sorter.Add(data[i]);
  sorter.SortAndFinish();

  TEST_EQUAL(serial.size(), data.size() * sizeof(data[0]), ());
  sort(data.begin(), data.end());
  MemReader reader(&serial[0], serial.size());
  TEST_EQUAL(reader.Size(), data.size() * sizeof(data[0]), ());
  vector<uint32_t> result(data.size());
  reader.Read(0, &result[0], reader.Size());
  TEST_EQUAL(result, data, ());
}
}  // namespace

UNIT_TEST(FileSorter_Smoke)
{
  vector<uint32_t> data;
  data.push_back(2);
  data.push_back(3);
  data.push_back(1);

  TestFileSorter(data, "file_sorter_test_smoke.tmp", 10);
}

UNIT_TEST(FileSorter_Random)
{
  mt19937 rng(0);
  vector<uint32_t> data(1000);
  for (size_t i = 0; i < data.size(); ++i)
    data[i] = ((i + 1 % 100) ? rng() : data[i - 20]);

  TestFileSorter(data, "file_sorter_test_random.tmp", data.size() / 10);
}
