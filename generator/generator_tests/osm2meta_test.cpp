#include <testing/testing.hpp>

#include <generator/osm2meta.hpp>

UNIT_TEST(ValidateAndFormat_ele)
{
  FeatureBuilderParams params;
  MetadataTagProcessorImpl tagProc(params);
  TEST_EQUAL(tagProc.ValidateAndFormat_ele(""), "", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("not a number"), "", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("0"), "0", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("0.0"), "0", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("0.0000000"), "0", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("22.5"), "22.5", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("-100.3"), "-100.3", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("99.0000000"), "99", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("8900.000023"), "8900", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("-300.9999"), "-301", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("-300.9"), "-300.9", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("15 m"), "15", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("15.9 m"), "15.9", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("15.9m"), "15.9", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("3000 ft"), "914.4", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("3000ft"), "914.4", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("100 feet"), "30.48", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("100feet"), "30.48", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("11'"), "3.35", ());
  TEST_EQUAL(tagProc.ValidateAndFormat_ele("11'4\""), "3.45", ());
}

UNIT_TEST(ValidateAndFormat_building_levels)
{
  FeatureBuilderParams params;
  MetadataTagProcessorImpl tp(params);
  TEST_EQUAL(tp.ValidateAndFormat_building_levels("４"), "4", ());
  TEST_EQUAL(tp.ValidateAndFormat_building_levels("４floors"), "4", ());
  TEST_EQUAL(tp.ValidateAndFormat_building_levels("between 1 and ４"), "", ());
  TEST_EQUAL(tp.ValidateAndFormat_building_levels("0"), "0", ("OSM has many zero-level buildings."));
  TEST_EQUAL(tp.ValidateAndFormat_building_levels("0.0"), "0", ());
  TEST_EQUAL(tp.ValidateAndFormat_building_levels(""), "", ());
  TEST_EQUAL(tp.ValidateAndFormat_building_levels("Level 1"), "", ());
  TEST_EQUAL(tp.ValidateAndFormat_building_levels("2.51"), "2.5", ());
  TEST_EQUAL(tp.ValidateAndFormat_building_levels("250"), "", ("Too many levels."));
}

UNIT_TEST(ValidateAndFormat_destination)
{
  FeatureBuilderParams params;
  MetadataTagProcessorImpl tp(params);
  TEST_EQUAL(tp.ValidateAndFormat_destination("a1 a2"), "a1 a2", ());
  TEST_EQUAL(tp.ValidateAndFormat_destination("b1-b2"), "b1-b2", ());
  TEST_EQUAL(tp.ValidateAndFormat_destination("  c,d ;"), "c; d", ());
  TEST_EQUAL(tp.ValidateAndFormat_destination("e,;f;  g;"), "e; f; g", ());
  TEST_EQUAL(tp.ValidateAndFormat_destination(""), "", ());
  TEST_EQUAL(tp.ValidateAndFormat_destination("a1 a2;b1-b2;  c,d ;e,;f;  ;g"), "a1 a2; b1-b2; c; d; e; f; g", ());
}