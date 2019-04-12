#include "testing/testing.hpp"

#include "coding/file_writer.hpp"
#include "coding/mmap_reader.hpp"
#include "coding/simple_dense_coding.hpp"
#include "coding/succinct_mapper.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

using namespace coding;
using namespace std;

namespace
{
void TestSDC(vector<uint8_t> const & data, SimpleDenseCoding const & coding)
{
  TEST_EQUAL(data.size(), coding.Size(), ());
  for (size_t i = 0; i < data.size(); ++i)
    TEST_EQUAL(data[i], coding.Get(i), ());
}
}  // namespace

UNIT_TEST(SimpleDenseCoding_Smoke)
{
  size_t const kSize = numeric_limits<uint8_t>::max();
  vector<uint8_t> data(kSize);
  for (size_t i = 0; i < data.size(); ++i)
    data[i] = i;

  string const kTestFile = "test.tmp";
  SCOPE_GUARD(cleanup, bind(&FileWriter::DeleteFileX, kTestFile));

  {
    SimpleDenseCoding coding(data);
    TestSDC(data, coding);
    FileWriter writer(kTestFile);
    Freeze(coding, writer, "SimpleDenseCoding");
  }

  {
    MmapReader reader(kTestFile);
    SimpleDenseCoding coding;
    Map(coding, reader.Data(), "SimpleDenseCoding");
    TestSDC(data, coding);
  }
}
