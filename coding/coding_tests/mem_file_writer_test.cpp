#include "testing/testing.hpp"

#include "coding/writer.hpp"
#include "coding/file_writer.hpp"

#include "base/macros.hpp"


UNIT_TEST(MemWriterEmpty)
{
  vector<char> data;
  {
    MemWriter< vector<char> > writer(data);
  }
  TEST(data.empty(), (data));
}

UNIT_TEST(MemWriterSimple)
{
  vector<char> data;
  MemWriter< vector<char> > writer(data);
  writer.Write("Hello", 5);
  writer.Write(",", 1);
  writer.Write("world!", 6);

  char const expected[] = "Hello,world!";
  TEST_EQUAL(data.size(), ARRAY_SIZE(expected)-1, ());
  TEST(equal(data.begin(), data.end(), &expected[0]), (data));
}
