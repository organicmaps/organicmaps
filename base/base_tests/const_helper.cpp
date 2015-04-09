#include "base/SRC_FIRST.hpp"
#include "testing/testing.hpp"

#include "base/const_helper.hpp"
#include "std/typeinfo.hpp"

UNIT_TEST(ConstHelper)
{
  TEST_EQUAL(typeid(my::PropagateConst<int, char>::type *).name(), typeid(char *).name(), ());
  TEST_EQUAL(typeid(my::PropagateConst<int, void>::type *).name(), typeid(void *).name(), ());
  TEST_EQUAL(typeid(my::PropagateConst<int const, char>::type *).name(),
             typeid(char const *).name(), ());
  TEST_EQUAL(typeid(my::PropagateConst<int const, void>::type *).name(),
             typeid(void const *).name(), ());
  TEST_EQUAL(typeid(my::PropagateConst<int, char const>::type *).name(),
             typeid(char const *).name(), ());
  TEST_EQUAL(typeid(my::PropagateConst<int, void const>::type *).name(),
             typeid(void const *).name(), ());
  TEST_EQUAL(typeid(my::PropagateConst<int const, char const>::type *).name(),
             typeid(char const *).name(), ());
  TEST_EQUAL(typeid(my::PropagateConst<int const, void const>::type *).name(),
             typeid(void const *).name(), ());
}
