#include <testing/testing.hpp>

#include <generator/osm2meta.hpp>

UNIT_TEST(ValidateAndFormat_cuisine_test)
{
  FeatureParams params;
  MetadataTagProcessorImpl tagProc(params);
  TEST_EQUAL(tagProc.ValidateAndFormat_cuisine(" ,ABC, CDE;   FgH,   "), "abc;cde;fgh", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_cuisine(";;;ABc,   cef,,,"), "abc;cef", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_cuisine("abc bca"), "abc_bca", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_cuisine("abc      def  gh"), "abc_def_gh", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_cuisine(""), "", ());
}
