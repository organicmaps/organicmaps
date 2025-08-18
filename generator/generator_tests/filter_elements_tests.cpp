#include "testing/testing.hpp"

#include "generator/filter_elements.hpp"
#include "generator/generator_tests/common.hpp"
#include "generator/osm_element.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

using namespace generator_tests;
using namespace generator;
using namespace platform::tests_support;

namespace
{
auto const kOsmElementEmpty = MakeOsmElement(0, {}, OsmElement::EntityType::Node);
auto const kOsmElementCity = MakeOsmElement(1, {{"place", "city"}, {"admin_level", "6"}}, OsmElement::EntityType::Node);
auto const kOsmElementCountry = MakeOsmElement(
    2, {{"admin_level", "2"}, {"ISO3166-1:alpha2", "RU"}, {"ISO3166-1:alpha3", "RUS"}, {"ISO3166-1:numeric", "643"}},
    OsmElement::EntityType::Relation);
}  // namespace

UNIT_TEST(FilterData_Ids)
{
  FilterData fd;
  fd.AddSkippedId(kOsmElementEmpty.m_id);

  TEST(fd.NeedSkipWithId(kOsmElementEmpty.m_id), ());
  TEST(!fd.NeedSkipWithId(kOsmElementCity.m_id), ());
}

UNIT_TEST(FilterData_Tags)
{
  FilterData fd;
  FilterData::Tags tags{{"admin_level", "2"}};

  fd.AddSkippedTags(tags);
  TEST(fd.NeedSkipWithTags(kOsmElementCountry.Tags()), ());
  TEST(!fd.NeedSkipWithTags(kOsmElementCity.Tags()), ());
}

UNIT_TEST(FilterElements_Case1)
{
  ScopedFile sf("tmp.txt",
                R"(
                {
                  "node": {
                    "ids": [1]
                  },
                  "relation": {
                    "tags": [{
                      "ISO3166-1:alpha2": "*"
                      }]
                  }
                }
                )");
  FilterElements fe(sf.GetFullPath());
  TEST(fe.NeedSkip(kOsmElementCity), ());
  TEST(!fe.NeedSkip(kOsmElementEmpty), ());
  TEST(fe.NeedSkip(kOsmElementCountry), ());
}

UNIT_TEST(FilterElements_Case2)
{
  ScopedFile sf("tmp.txt",
                R"(
                {
                  "node": {
                    "ids": [0, 1]
                  }
                }
                )");
  FilterElements fe(sf.GetFullPath());
  TEST(fe.NeedSkip(kOsmElementCity), ());
  TEST(!fe.NeedSkip(kOsmElementCountry), ());
  TEST(fe.NeedSkip(kOsmElementEmpty), ());
}

UNIT_TEST(FilterElements_Case3)
{
  ScopedFile sf("tmp.txt",
                R"(
                {
                  "node": {
                    "tags": [{
                      "admin_level": "*"
                      }]
                  },
                  "relation": {
                    "tags": [{
                      "admin_level": "*"
                      }]
                  }
                }
                )");
  FilterElements fe(sf.GetFullPath());
  TEST(fe.NeedSkip(kOsmElementCity), ());
  TEST(fe.NeedSkip(kOsmElementCountry), ());
  TEST(!fe.NeedSkip(kOsmElementEmpty), ());
}

UNIT_TEST(FilterElements_Case4)
{
  ScopedFile sf("tmp.txt",
                R"(
                {
                }
                )");
  FilterElements fe(sf.GetFullPath());
  TEST(!fe.NeedSkip(kOsmElementCity), ());
  TEST(!fe.NeedSkip(kOsmElementCountry), ());
  TEST(!fe.NeedSkip(kOsmElementEmpty), ());
}
