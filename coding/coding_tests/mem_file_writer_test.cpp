#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"

#include "../writer.hpp"
#include "../../base/macros.hpp"

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
  char const expected_string[] = "Hello,world!";
  vector<char> expected_data(&expected_string[0], &expected_string[ARRAY_SIZE(expected_string)-1]);
  TEST_EQUAL(data, expected_data, ());
}
