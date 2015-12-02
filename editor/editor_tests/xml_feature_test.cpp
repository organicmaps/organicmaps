#include "testing/testing.hpp"

#include "editor/xml_feature.hpp"

#include "base/timer.hpp"

#include "std/sstream.hpp"

#include "3party/pugixml/src/pugixml.hpp"

using namespace indexer;

UNIT_TEST(XMLFeature_RawGetSet)
{
  XMLFeature feature;
  TEST(!feature.HasTag("opening_hours"), ());
  TEST(!feature.HasAttribute("center"), ());

  feature.SetAttribute("FooBar", "foobar");
  TEST_EQUAL(feature.GetAttribute("FooBar"), "foobar", ());

  feature.SetAttribute("FooBar", "foofoo");
  TEST_EQUAL(feature.GetAttribute("FooBar"), "foofoo", ());

  feature.SetTagValue("opening_hours", "18:20-18:21");
  TEST_EQUAL(feature.GetTagValue("opening_hours"), "18:20-18:21", ());

  feature.SetTagValue("opening_hours", "18:20-19:21");
  TEST_EQUAL(feature.GetTagValue("opening_hours"), "18:20-19:21", ());

  auto const expected = R"(<?xml version="1.0"?>
<node
  FooBar="foofoo">
  <tag
    k="opening_hours"
    v="18:20-19:21" />
</node>
)";

  stringstream sstr;
  feature.Save(sstr);
  TEST_EQUAL(expected, sstr.str(), ());
}


UNIT_TEST(XMLFeature_Setters)
{
  XMLFeature feature;

  feature.SetCenter(m2::PointD(64.2342340, 53.3124200));
  feature.SetModificationTime(my::StringToTimestamp("2015-11-27T21:13:32Z"));

  feature.SetName("Gorki Park");
  feature.SetName("en", "Gorki Park");
  feature.SetName("ru", "Парк Горького");

  feature.SetHouse("10");
  feature.SetTagValue("opening_hours", "Mo-Fr 08:15-17:30");

  stringstream sstr;
  feature.Save(sstr);

  auto const expectedString = R"(<?xml version="1.0"?>
<node
  center="64.2342340, 53.3124200"
  timestamp="2015-11-27T21:13:32Z">
  <tag
    k="name"
    v="Gorki Park" />
  <tag
    k="name:en"
    v="Gorki Park" />
  <tag
    k="name:ru"
    v="Парк Горького" />
  <tag
    k="addr:housenumber"
    v="10" />
  <tag
    k="opening_hours"
    v="Mo-Fr 08:15-17:30" />
</node>
)";

  TEST_EQUAL(sstr.str(), expectedString, ());
}

UNIT_TEST(XMLFeatureFromXml)
{
  auto const srcString = R"(<?xml version="1.0"?>
<node
  center="64.2342340, 53.3124200"
  timestamp="2015-11-27T21:13:32Z">
  <tag
    k="name"
    v="Gorki Park" />
  <tag
    k="name:en"
    v="Gorki Park" />
  <tag
    k="name:ru"
    v="Парк Горького" />
  <tag
    k="addr:housenumber"
    v="10" />
  <tag
    k="opening_hours"
    v="Mo-Fr 08:15-17:30" />
</node>
)";

  XMLFeature feature(srcString);

  stringstream sstr;
  feature.Save(sstr);
  TEST_EQUAL(srcString, sstr.str(), ());

  TEST(feature.HasKey("opening_hours"), ());
  TEST(feature.HasKey("center"), ());
  TEST(!feature.HasKey("FooBarBaz"), ());

  TEST_EQUAL(feature.GetHouse(), "10", ());
  TEST_EQUAL(feature.GetCenter(), m2::PointD(64.2342340, 53.3124200), ());

  TEST_EQUAL(feature.GetName(), "Gorki Park", ());
  TEST_EQUAL(feature.GetName("default"), "Gorki Park", ());
  TEST_EQUAL(feature.GetName("en"), "Gorki Park", ());
  TEST_EQUAL(feature.GetName("ru"), "Парк Горького", ());
  TEST_EQUAL(feature.GetName("No such language"), "", ());

  TEST_EQUAL(feature.GetTagValue("opening_hours"), "Mo-Fr 08:15-17:30", ());
  TEST_EQUAL(my::TimestampToString(feature.GetModificationTime()), "2015-11-27T21:13:32Z", ());
}
