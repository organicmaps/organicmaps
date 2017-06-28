#include "testing/testing.hpp"

#include "indexer/classificator.hpp"

namespace
{
  void check_values_array(uint8_t values[], uint8_t count)
  {
    uint32_t type = ftype::GetEmptyValue();
    uint8_t value;
    bool res = ftype::GetValue(type, 0, value);
    TEST_EQUAL(res, false, ());
    res = ftype::GetValue(type, 4, value);
    TEST_EQUAL(res, false, ());

    for (uint8_t i = 0; i < count; ++i)
      ftype::PushValue(type, values[i]);

    for (uint8_t i = 0; i < count; ++i)
    {
      res = ftype::GetValue(type, i, value);
      TEST_EQUAL(res, true, ());
      TEST_EQUAL(value, values[i], (value, values[i]));
    }

    for (char i = count-1; i >= 0; --i)
    {
      ftype::PopValue(type);

      res = ftype::GetValue(type, i, value);
      TEST_EQUAL(res, false, ());
    }

    TEST_EQUAL(type, ftype::GetEmptyValue(), (type));
  }
}

UNIT_TEST(SetGetTypes)
{
  uint8_t v1[] = { 6, 30, 0, 1 };
  check_values_array(v1, 4);
  check_values_array(v1, 3);

  uint8_t v2[] = { 0, 0, 0, 0 };
  check_values_array(v2, 4);
  check_values_array(v2, 3);

  uint8_t v3[] = { 1, 1, 1, 1 };
  check_values_array(v3, 4);
  check_values_array(v3, 3);

  uint8_t v4[] = { 63, 63, 63, 63 };
  check_values_array(v4, 4);
  check_values_array(v4, 3);
}
