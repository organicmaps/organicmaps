#include "testing/testing.hpp"

#include "base/macros.hpp"
#include "coding/reader.hpp"

UNIT_TEST(MemReaderSimple)
{
  char constexpr data[] = "123";
  size_t constexpr n = ARRAY_SIZE(data);
  MemReader const memReader(data, n);
  for (size_t i = 0; i < n; ++i)
  {
    uint8_t c[4] = {0xff, 0xff, 0xff, 0xff};
    ReadFromPos(memReader, i, c, n - i);
    for (size_t j = 0; j < n; ++j)
      TEST_EQUAL(c[j], i + j < n ? data[i + j] : uint8_t(0xff), (i, j, n));
  }
  MemReader const subReader = memReader.SubReader(1, n - 2);
  for (size_t i = 1; i < n - 1; ++i)
  {
    uint8_t c[4] = {0xff, 0xff, 0xff, 0xff};
    ReadFromPos(subReader, i, c, n - i - 2);
    for (size_t j = 0; j < n; ++j)
      TEST_EQUAL(c[j], i + j < n - 2 ? data[i + j + 1] : uint8_t(0xff), (i, j, n));
  }
}

UNIT_TEST(MemReaderStringView)
{
  std::string_view constexpr data = "1234567";
  MemReader const memReader(data);
  size_t constexpr n = data.size();
  for (size_t i = 0; i < n; ++i)
  {
    uint8_t c[n] = {0};
    ReadFromPos(memReader, i, c, n - i);
    for (size_t j = 0; j < n; ++j)
      TEST_EQUAL(c[j], i + j < n ? data[i + j] : uint8_t{0}, (i, j, n));
  }
  MemReader const subReader = memReader.SubReader(1, n - 2);
  for (size_t i = 1; i < n - 1; ++i)
  {
    uint8_t c[n] = {0};
    ReadFromPos(subReader, i, c, n - i - 2);
    for (size_t j = 0; j < n; ++j)
      TEST_EQUAL(c[j], i + j < n - 2 ? data[i + j + 1] : uint8_t{0}, (i, j, n));
  }
}
