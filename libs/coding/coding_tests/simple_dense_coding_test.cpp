#include "testing/testing.hpp"

#include "coding/file_writer.hpp"
#include "coding/mmap_reader.hpp"
#include "coding/simple_dense_coding.hpp"
#include "coding/succinct_mapper.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <limits>
#include <random>
#include <string>
#include <vector>

namespace simple_dense_coding_test
{
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

UNIT_TEST(SimpleDenseCoding_Ratio)
{
  for (uint8_t const maxValue : {16, 32, 64})
  {
    size_t constexpr kSize = 1 << 20;

    normal_distribution<> randDist(maxValue / 2, 2);
    random_device randDevice;
    mt19937 randEngine(randDevice());

    vector<uint8_t> data(kSize);
    for (size_t i = 0; i < kSize; ++i)
    {
      double d = round(randDist(randEngine));
      if (d < 0)
        d = 0;
      else if (d > maxValue)
        d = maxValue;
      data[i] = static_cast<uint8_t>(d);
    }

    SimpleDenseCoding coding(data);
    TestSDC(data, coding);

    vector<uint8_t> buffer;
    MemWriter writer(buffer);
    Freeze(coding, writer, "");

    auto const ratio = data.size() / double(buffer.size());
    LOG(LINFO, (maxValue, ratio));
    TEST_GREATER(ratio, 1.8, ());
  }
}
}  // namespace simple_dense_coding_test
