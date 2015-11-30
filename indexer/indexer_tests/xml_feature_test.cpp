#include "testing/testing.hpp"

#include "indexer/xml_feature.hpp"

#include "3party/pugixml/src/pugixml.hpp"

#include "base/timer.hpp"

#include "std/sstream.hpp"

// UNIT_TEST(FeatureFromXml)
// {
//   auto const featureXml = R"(
//     <node id="1107716196" visible="true" version="5" changeset="7864398"
//           timestamp="2011-04-14T19:51:12Z" user="Amigo" uid="206282"
//           lat="55.7978998" lon="37.4745280">
//       <tag k="historic" v="memorial"/>
//       <tag k="name" v="Памятник В.И. Ленину"/>
//     </node>)";

//   pugi::xml_document doc;
//   auto const result = doc.load_string(featureXml);
//   TEST(result, (result.description()));
// }

using namespace indexer;

// UNIT_TEST(FeatureToXml)
// {
//   XMLFeature feature;
//   feature.SetModificationTime(my::StringToTimestamp("2015-11-27T21:13:32Z"));

//   feature.SetCenter({64.234234, 53.31242});
//   feature.SetTagValue("opening_hours", "Mo-Fr 08:15-17:30");

//   feature.SetInterationalName("Gorki Park");

//   StringUtf8Multilang names;
//   names.AddString("en", "Gorki Park");
//   names.AddString("ru", "Парк Горького");

//   feature.SetMultilungName(names);

//   StringNumericOptimal house;
//   house.Set(10);
//   feature.SetHouse(house);

//   pugi::xml_document document;
//   feature.ToXMLDocument(document);

//   stringstream sstr;
//   document.save(sstr, "\t", pugi::format_indent_attributes);

//   auto const expectedString = R"(<?xml version="1.0"?>
// <node
// 	center="64.234234000000000719, 53.312420000000003029"
// 	timestamp="2015-11-27T21:13:32Z">
// 	<tag
// 		k="name"
// 		v="Gorki Park" />
// 	<tag
// 		k="name::en"
// 		v="Gorki Park" />
// 	<tag
// 		k="name::ru"
// 		v="Парк Горького" />
// 	<tag
// 		k="addr:housenumber"
// 		v="10" />
// 	<tag
// 		k="opening_hours"
// 		v="Mo-Fr 08:15-17:30" />
// </node>
// )";

//   TEST_EQUAL(sstr.str(), expectedString, ());
// }


UNIT_TEST(FeatureFromXml)
{
  auto const srcString = R"(<?xml version="1.0"?>
<node
	center="64.2342340, 53.3124200"
	timestamp="2015-11-27T21:13:32Z">
	<tag
		k="name"
		v="Gorki Park" />
	<tag
		k="name::en"
		v="Gorki Park" />
	<tag
		k="name::ru"
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
  feature.GetXMLDocument().save(sstr, "\t", pugi::format_indent_attributes);
  TEST_EQUAL(srcString, sstr.str(), ());

  TEST(feature.HasTag("opening_hours"), ());
  TEST(!feature.HasTag("FooBarBaz"), ());

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
