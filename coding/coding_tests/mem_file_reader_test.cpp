#include "testing/testing.hpp"

#include "coding/reader.hpp"
#include "base/macros.hpp"

UNIT_TEST(MemReaderSimple)
{
  char const data[] = "123";
  size_t const n = ARRAY_SIZE(data);
  MemReader memReader(data, n);
  for (size_t i = 0; i < n; ++i)
  {
    uint8_t c[4] = {0xff, 0xff, 0xff, 0xff};
    ReadFromPos(memReader, i, c, n - i);
    for (size_t j = 0; j < n; ++j)
      TEST_EQUAL(c[j], i + j < n ? data[i + j] : uint8_t(0xff), (i, j, n));
  }
  MemReader subReader = memReader.SubReader(1, 2);
  for (size_t i = 1; i < n - 1; ++i)
  {
    uint8_t c[4] = {0xff, 0xff, 0xff, 0xff};
    ReadFromPos(subReader, i, c, n - i - 2);
    for (size_t j = 0; j < n; ++j)
      TEST_EQUAL(c[j], i + j < n - 2? data[i + j + 1] : uint8_t(0xff), (i, j, n));
  }
}
