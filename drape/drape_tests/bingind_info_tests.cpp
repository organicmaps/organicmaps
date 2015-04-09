#include "testing/testing.hpp"

#include "drape/binding_info.hpp"

using namespace dp;

UNIT_TEST(BindingInfoIDTest)
{
  {
    BindingInfo info(1, 1);
    TEST_EQUAL(info.GetID(), 1, ());
  }

  {
    BindingInfo info(1);
    TEST_EQUAL(info.GetID(), 0, ());
  }
}

UNIT_TEST(DynamicHandlingTest)
{
  {
    BindingInfo info(1);
    TEST_EQUAL(info.IsDynamic(), false, ());
  }

  {
    BindingInfo info(1, 1);
    TEST_EQUAL(info.IsDynamic(), true, ());
  }
}
