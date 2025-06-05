#include "testing/testing.hpp"

#include "indexer/classificator.hpp"

namespace
{
void check_values_array(uint8_t values[], uint8_t count)
{
  uint32_t type = ftype::GetEmptyValue();

  for (uint8_t i = 0; i < count; ++i)
    ftype::PushValue(type, values[i]);

  for (uint8_t i = 0; i < count; ++i)
    TEST_EQUAL(ftype::GetValue(type, i), values[i], ());

  while (count > 0)
  {
    TEST_EQUAL(ftype::GetLevel(type), count, ());
    ftype::PopValue(type);
    --count;
  }

  TEST_EQUAL(ftype::GetLevel(type), 0, ());
  TEST_EQUAL(type, ftype::GetEmptyValue(), (type));
}
}  // namespace

UNIT_TEST(SetGetTypes)
{
  uint8_t v1[] = {6, 30, 0, 1};
  check_values_array(v1, 4);
  check_values_array(v1, 3);

  uint8_t v2[] = {0, 0, 0, 0};
  check_values_array(v2, 4);
  check_values_array(v2, 3);

  uint8_t v3[] = {1, 1, 1, 1};
  check_values_array(v3, 4);
  check_values_array(v3, 3);

  uint8_t v4[] = {63, 63, 63, 63};
  check_values_array(v4, 4);
  check_values_array(v4, 3);
}
