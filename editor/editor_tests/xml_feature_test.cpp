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

UNIT_TEST(XMLFeature_ToOSMString)
{
  XMLFeature feature(XMLFeature::Type::Node);
  feature.SetCenter(MercatorBounds::FromLatLon(55.7978998, 37.4745280));
  feature.SetName("OSM");
  feature.SetTagValue("amenity", "atm");

  auto const expectedString = R"(<?xml version="1.0"?>
<osm>
<node lat="55.7978998" lon="37.474528">
  <tag k="name" v="OSM" />
  <tag k="amenity" v="atm" />
</node>
</osm>
)";
  TEST_EQUAL(expectedString, feature.ToOSMString(), ());
}

UNIT_TEST(XMLFeature_IsArea)
{
  constexpr char const * validAreaXml = R"(
<way timestamp="2015-11-27T21:13:32Z">
  <nd ref="822403"/>
  <nd ref="21533912"/>
  <nd ref="821601"/>
  <nd ref="822403"/>
</way>
)";
  TEST(XMLFeature(validAreaXml).IsArea(), ());

  constexpr char const * notClosedWayXml = R"(
<way timestamp="2015-11-27T21:13:32Z">
  <nd ref="822403"/>
  <nd ref="21533912"/>
  <nd ref="821601"/>
  <nd ref="123321"/>
</way>
)";
  TEST(!XMLFeature(notClosedWayXml).IsArea(), ());

  constexpr char const * invalidWayXml = R"(
<way timestamp="2015-11-27T21:13:32Z">
  <nd ref="822403"/>
  <nd ref="21533912"/>
  <nd ref="822403"/>
</way>
)";
  TEST(!XMLFeature(invalidWayXml).IsArea(), ());

  constexpr char const * emptyWay = R"(
<way timestamp="2015-11-27T21:13:32Z"/>
)";
  TEST(!XMLFeature(emptyWay).IsArea(), ());

  constexpr char const * node = R"(
<node lat="0.0" lon="0.0" timestamp="2015-11-27T21:13:32Z"/>
)";
  TEST(!XMLFeature(node).IsArea(), ());
}

auto const kTestNode = R"(<?xml version="1.0"?>
<node lat="55.7978998" lon="37.474528" timestamp="2015-11-27T21:13:32Z">
  <tag k="name" v="Gorki Park" />
  <tag k="name:en" v="Gorki Park" />
  <tag k="name:ru" v="Парк Горького" />
  <tag k="addr:housenumber" v="10" />
  <tag k="opening_hours" v="Mo-Fr 08:15-17:30" />
  <tag k="amenity" v="atm" />
</node>
)";

UNIT_TEST(XMLFeature_FromXml)
{
  XMLFeature feature(kTestNode);

  stringstream sstr;
  feature.Save(sstr);
  TEST_EQUAL(kTestNode, sstr.str(), ());

  TEST(feature.HasKey("opening_hours"), ());
  TEST(feature.HasKey("lat"), ());
  TEST(feature.HasKey("lon"), ());
  TEST(!feature.HasKey("FooBarBaz"), ());

  TEST_EQUAL(feature.GetHouse(), "10", ());
  TEST_EQUAL(feature.GetCenter(), ms::LatLon(55.7978998, 37.4745280), ());
  TEST_EQUAL(feature.GetName(), "Gorki Park", ());
  TEST_EQUAL(feature.GetName("default"), "Gorki Park", ());
  TEST_EQUAL(feature.GetName("en"), "Gorki Park", ());
  TEST_EQUAL(feature.GetName("ru"), "Парк Горького", ());
  TEST_EQUAL(feature.GetName("No such language"), "", ());

  TEST_EQUAL(feature.GetTagValue("opening_hours"), "Mo-Fr 08:15-17:30", ());
  TEST_EQUAL(feature.GetTagValue("amenity"), "atm", ());
  TEST_EQUAL(my::TimestampToString(feature.GetModificationTime()), "2015-11-27T21:13:32Z", ());
}

UNIT_TEST(XMLFeature_ForEachName)
{
  XMLFeature feature(kTestNode);
  map<string, string> names;

  feature.ForEachName([&names](string const & lang, string const & name)
                      {
                        names.emplace(lang, name);
                      });

  TEST_EQUAL(names, (map<string, string>{
                        {"default", "Gorki Park"}, {"en", "Gorki Park"}, {"ru", "Парк Горького"}}),
             ());
}

UNIT_TEST(XMLFeature_FromOSM)
{
  auto const kTestNodeWay = R"(<?xml version="1.0"?>
  <osm>
  <node id="4" lat="55.7978998" lon="37.474528" timestamp="2015-11-27T21:13:32Z">
    <tag k="test" v="value"/>
  </node>
  <node id="5" lat="55.7977777" lon="37.474528" timestamp="2015-11-27T21:13:33Z"/>
  <way id="3" timestamp="2015-11-27T21:13:34Z">
    <nd ref="4"/>
    <nd ref="5"/>
    <tag k="hi" v="test"/>
  </way>
  </osm>
  )";

  TEST_ANY_THROW(XMLFeature::FromOSM(""), ());
  TEST_ANY_THROW(XMLFeature::FromOSM("This is not XML"), ());
  TEST_ANY_THROW(XMLFeature::FromOSM("<?xml version=\"1.0\"?>"), ());
  TEST_NO_THROW(XMLFeature::FromOSM("<?xml version=\"1.0\"?><osm></osm>"), ());
  TEST_ANY_THROW(XMLFeature::FromOSM("<?xml version=\"1.0\"?><osm><node lat=\"11.11\"/></osm>"), ());
  vector<XMLFeature> features;
  TEST_NO_THROW(features = XMLFeature::FromOSM(kTestNodeWay), ());
  TEST_EQUAL(3, features.size(), ());
  XMLFeature const & node = features[0];
  TEST_EQUAL(node.GetAttribute("id"), "4", ());
  TEST_EQUAL(node.GetTagValue("test"), "value", ());
  TEST_EQUAL(features[2].GetTagValue("hi"), "test", ());
}

UNIT_TEST(XMLFeature_FromXmlNode)
{
  auto const kTestNode = R"(<?xml version="1.0"?>
  <osm>
  <node id="4" lat="55.7978998" lon="37.474528" timestamp="2015-11-27T21:13:32Z">
    <tag k="amenity" v="fountain"/>
  </node>
  </osm>
  )";

  pugi::xml_document doc;
  doc.load_string(kTestNode);
  XMLFeature const feature(doc.child("osm").child("node"));
  TEST_EQUAL(feature.GetAttribute("id"), "4", ());
  TEST_EQUAL(feature.GetTagValue("amenity"), "fountain", ());
  XMLFeature copy(feature);
  TEST_EQUAL(copy.GetAttribute("id"), "4", ());
  TEST_EQUAL(copy.GetTagValue("amenity"), "fountain", ());
}
