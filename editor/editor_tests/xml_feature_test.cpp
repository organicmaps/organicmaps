#include "testing/testing.hpp"

#include "editor/xml_feature.hpp"

#include "geometry/mercator.hpp"

#include "base/timer.hpp"

#include "std/map.hpp"
#include "std/sstream.hpp"

#include "3party/pugixml/src/pugixml.hpp"

using namespace editor;

UNIT_TEST(XMLFeature_RawGetSet)
{
  XMLFeature feature(XMLFeature::Type::Node);
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
<node FooBar="foofoo">
  <tag k="opening_hours" v="18:20-19:21" />
</node>
)";

  stringstream sstr;
  feature.Save(sstr);
  TEST_EQUAL(expected, sstr.str(), ());
}


UNIT_TEST(XMLFeature_Setters)
{
  XMLFeature feature(XMLFeature::Type::Node);

  feature.SetCenter(MercatorBounds::FromLatLon(55.7978998, 37.4745280));
  feature.SetModificationTime(my::StringToTimestamp("2015-11-27T21:13:32Z"));

  feature.SetName("Gorki Park");
  feature.SetName("en", "Gorki Park");
  feature.SetName("ru", "Парк Горького");

  feature.SetHouse("10");
  feature.SetTagValue("opening_hours", "Mo-Fr 08:15-17:30");
  feature.SetTagValue("amenity", "atm");

  stringstream sstr;
  feature.Save(sstr);

  auto const expectedString = R"(<?xml version="1.0"?>
<node lat="55.7978998" lon="37.474528" timestamp="2015-11-27T21:13:32Z">
  <tag k="name" v="Gorki Park" />
  <tag k="name:en" v="Gorki Park" />
  <tag k="name:ru" v="Парк Горького" />
  <tag k="addr:housenumber" v="10" />
  <tag k="opening_hours" v="Mo-Fr 08:15-17:30" />
  <tag k="amenity" v="atm" />
</node>
)";

  TEST_EQUAL(sstr.str(), expectedString, ());
}

// UNIT_TEST(XMLFeature_FromXml)
// {
//   auto const srcString = R"(<?xml version="1.0"?>
// <node
//   lat="55.7978998"
//   lon="37.474528"
//   timestamp="2015-11-27T21:13:32Z">
//   <tag
//     k="name"
//     v="Gorki Park" />
//   <tag
//     k="name:en"
//     v="Gorki Park" />
//   <tag
//     k="name:ru"
//     v="Парк Горького" />
//   <tag
//     k="addr:housenumber"
//     v="10" />
//   <tag
//     k="opening_hours"
//     v="Mo-Fr 08:15-17:30" />
//   <tag
//     k="amenity"
//     v="atm" />
// </node>
// )";

//   XMLFeature feature(srcString);

//   stringstream sstr;
//   feature.Save(sstr);
//   TEST_EQUAL(srcString, sstr.str(), ());

//   TEST(feature.HasKey("opening_hours"), ());
//   TEST(feature.HasKey("lat"), ());
//   TEST(feature.HasKey("lon"), ());
//   TEST(!feature.HasKey("FooBarBaz"), ());

//   TEST_EQUAL(feature.GetHouse(), "10", ());
//   TEST_EQUAL(feature.GetCenter(), MercatorBounds::FromLatLon(55.7978998, 37.4745280), ());
//   TEST_EQUAL(feature.GetName(), "Gorki Park", ());
//   TEST_EQUAL(feature.GetName("default"), "Gorki Park", ());
//   TEST_EQUAL(feature.GetName("en"), "Gorki Park", ());
//   TEST_EQUAL(feature.GetName("ru"), "Парк Горького", ());
//   TEST_EQUAL(feature.GetName("No such language"), "", ());

//   TEST_EQUAL(feature.GetTagValue("opening_hours"), "Mo-Fr 08:15-17:30", ());
//   TEST_EQUAL(feature.GetTagValue("amenity"), "atm", ());
//   TEST_EQUAL(my::TimestampToString(feature.GetModificationTime()), "2015-11-27T21:13:32Z", ());
// }

UNIT_TEST(XMLFeature_ForEachName)
{
  auto const srcString = R"(<?xml version="1.0"?>
<node lat="55.7978998" lon="37.474528" timestamp="2015-11-27T21:13:32Z">
  <tag k="name" v="Gorki Park" />
  <tag k="name:en" v="Gorki Park" />
  <tag k="name:ru" v="Парк Горького" />
  <tag k="addr:housenumber" v="10" />
  <tag k="opening_hours" v="Mo-Fr 08:15-17:30" />
  <tag k="amenity" v="atm" />
</node>
)";

  XMLFeature feature(srcString);
  map<string, string> names;

  feature.ForEachName([&names](string const & lang, string const & name)
                      {
                        names.emplace(lang, name);
                      });

  TEST_EQUAL(names, (map<string, string>{
                        {"default", "Gorki Park"}, {"en", "Gorki Park"}, {"ru", "Парк Горького"}}),
             ());
}
