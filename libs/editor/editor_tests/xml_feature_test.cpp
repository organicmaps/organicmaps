#include "testing/testing.hpp"

#include "editor/xml_feature.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/editable_map_object.hpp"

#include "geometry/mercator.hpp"

#include "base/timer.hpp"

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <pugixml.hpp>

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

  std::stringstream sstr;
  feature.Save(sstr);
  TEST_EQUAL(expected, sstr.str(), ());
}

UNIT_TEST(XMLFeature_Setters)
{
  XMLFeature feature(XMLFeature::Type::Node);

  feature.SetCenter(mercator::FromLatLon(55.7978998, 37.4745280));
  feature.SetModificationTime(base::StringToTimestamp("2015-11-27T21:13:32Z"));

  feature.SetName("Gorki Park");
  feature.SetName("en", "Gorki Park");
  feature.SetName("ru", "Парк Горького");
  feature.SetName("int_name", "Gorky Park");

  feature.SetHouse("10");
  feature.SetTagValue("opening_hours", "Mo-Fr 08:15-17:30");
  feature.SetTagValue("amenity", "atm");

  std::stringstream sstr;
  feature.Save(sstr);

  auto const expectedString = R"(<?xml version="1.0"?>
<node lat="55.7978998" lon="37.474528" timestamp="2015-11-27T21:13:32Z">
  <tag k="name" v="Gorki Park" />
  <tag k="name:en" v="Gorki Park" />
  <tag k="name:ru" v="Парк Горького" />
  <tag k="int_name" v="Gorky Park" />
  <tag k="addr:housenumber" v="10" />
  <tag k="opening_hours" v="Mo-Fr 08:15-17:30" />
  <tag k="amenity" v="atm" />
</node>
)";

  TEST_EQUAL(sstr.str(), expectedString, ());
}

UNIT_TEST(XMLFeature_UintLang)
{
  XMLFeature feature(XMLFeature::Type::Node);

  feature.SetCenter(mercator::FromLatLon(55.79, 37.47));
  feature.SetModificationTime(base::StringToTimestamp("2015-11-27T21:13:32Z"));

  feature.SetName(StringUtf8Multilang::kDefaultCode, "Gorki Park");
  feature.SetName(StringUtf8Multilang::GetLangIndex("ru"), "Парк Горького");
  feature.SetName(StringUtf8Multilang::kInternationalCode, "Gorky Park");
  std::stringstream sstr;
  feature.Save(sstr);

  auto const expectedString = R"(<?xml version="1.0"?>
<node lat="55.79" lon="37.47" timestamp="2015-11-27T21:13:32Z">
  <tag k="name" v="Gorki Park" />
  <tag k="name:ru" v="Парк Горького" />
  <tag k="int_name" v="Gorky Park" />
</node>
)";

  TEST_EQUAL(sstr.str(), expectedString, ());

  XMLFeature f2(expectedString);
  TEST_EQUAL(f2.GetName(StringUtf8Multilang::kDefaultCode), "Gorki Park", ());
  TEST_EQUAL(f2.GetName(StringUtf8Multilang::GetLangIndex("ru")), "Парк Горького", ());
  TEST_EQUAL(f2.GetName(StringUtf8Multilang::kInternationalCode), "Gorky Park", ());

  TEST_EQUAL(f2.GetName(), "Gorki Park", ());
  TEST_EQUAL(f2.GetName("default"), "Gorki Park", ());
  TEST_EQUAL(f2.GetName("ru"), "Парк Горького", ());
  TEST_EQUAL(f2.GetName("int_name"), "Gorky Park", ());
}

UNIT_TEST(XMLFeature_ToOSMString)
{
  XMLFeature feature(XMLFeature::Type::Node);
  feature.SetCenter(mercator::FromLatLon(55.7978998, 37.4745280));
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

UNIT_TEST(XMLFeature_HasTags)
{
  auto const taggedNode = R"(
<node lat="55.7978998" lon="37.474528" timestamp="2015-11-27T21:13:32Z">
  <tag k="name" v="OSM" />
  <tag k="amenity" v="atm" />
</node>
)";
  XMLFeature taggedFeature(taggedNode);
  TEST(taggedFeature.HasAnyTags(), ());
  TEST(taggedFeature.HasTag("amenity"), ());
  TEST(taggedFeature.HasKey("amenity"), ());
  TEST(!taggedFeature.HasTag("name:en"), ());
  TEST(taggedFeature.HasKey("lon"), ());
  TEST(!taggedFeature.HasTag("lon"), ());
  TEST_EQUAL(taggedFeature.GetTagValue("name"), "OSM", ());
  TEST_EQUAL(taggedFeature.GetTagValue("nope"), "", ());

  constexpr char const * emptyWay = R"(
<way timestamp="2015-11-27T21:13:32Z"/>
)";
  XMLFeature emptyFeature(emptyWay);
  TEST(!emptyFeature.HasAnyTags(), ());
  TEST(emptyFeature.HasAttribute("timestamp"), ());
}

UNIT_TEST(XMLFeature_FromXml)
{
  // Do not space-align this string literal constant. It will be compared below.
  auto const kTestNode = R"(<?xml version="1.0"?>
<node lat="55.7978998" lon="37.474528" timestamp="2015-11-27T21:13:32Z">
  <tag k="name" v="Gorki Park" />
  <tag k="name:en" v="Gorki Park" />
  <tag k="name:ru" v="Парк Горького" />
  <tag k="int_name" v="Gorky Park" />
  <tag k="addr:housenumber" v="10" />
  <tag k="opening_hours" v="Mo-Fr 08:15-17:30" />
  <tag k="amenity" v="atm" />
</node>
)";

  std::map<std::string_view, std::string_view> kTestNames{
      {"default", "Gorki Park"}, {"en", "Gorki Park"}, {"ru", "Парк Горького"}, {"int_name", "Gorky Park"}};

  XMLFeature feature(kTestNode);

  std::stringstream sstr;
  feature.Save(sstr);
  TEST_EQUAL(kTestNode, sstr.str(), ());

  TEST(feature.HasKey("opening_hours"), ());
  TEST(feature.HasKey("lat"), ());
  TEST(feature.HasKey("lon"), ());
  TEST(!feature.HasKey("FooBarBaz"), ());

  TEST_EQUAL(feature.GetHouse(), "10", ());
  TEST_EQUAL(feature.GetCenter(), ms::LatLon(55.7978998, 37.4745280), ());
  TEST_EQUAL(feature.GetName(), kTestNames["default"], ());
  TEST_EQUAL(feature.GetName("default"), kTestNames["default"], ());
  TEST_EQUAL(feature.GetName("en"), kTestNames["en"], ());
  TEST_EQUAL(feature.GetName("ru"), kTestNames["ru"], ());
  TEST_EQUAL(feature.GetName("int_name"), kTestNames["int_name"], ());
  TEST_EQUAL(feature.GetName("No such language"), "", ());

  TEST_EQUAL(feature.GetTagValue("opening_hours"), "Mo-Fr 08:15-17:30", ());
  TEST_EQUAL(feature.GetTagValue("amenity"), "atm", ());
  TEST_EQUAL(base::TimestampToString(feature.GetModificationTime()), "2015-11-27T21:13:32Z", ());

  std::map<std::string_view, std::string_view> names;
  feature.ForEachName([&names](std::string_view lang, std::string_view name) { names.emplace(lang, name); });
  TEST_EQUAL(names, kTestNames, ());
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
  std::vector<XMLFeature> features;
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
  XMLFeature const copy(feature);
  TEST_EQUAL(copy.GetAttribute("id"), "4", ());
  TEST_EQUAL(copy.GetTagValue("amenity"), "fountain", ());
}

UNIT_TEST(XMLFeature_Geometry)
{
  std::vector<m2::PointD> const geometry = {
      {28.7206411, 3.7182409},  {46.7569003, 47.0774689}, {22.5909217, 41.6994874}, {14.7537008, 17.7788229},
      {55.1261701, 10.3199476}, {28.6519654, 50.0305930}, {28.7206411, 3.7182409}};

  XMLFeature feature(XMLFeature::Type::Way);
  feature.SetGeometry(geometry);
  TEST_EQUAL(feature.GetGeometry(), geometry, ());
}

UNIT_TEST(XMLFeature_ApplyPatch)
{
  auto const kOsmFeature = R"(<?xml version="1.0"?>
  <osm>
  <node id="1" lat="1" lon="2" timestamp="2015-11-27T21:13:32Z" version="1">
    <tag k="amenity" v="cafe"/>
  </node>
  </osm>
  )";

  auto const kPatch = R"(<?xml version="1.0"?>
  <node lat="1" lon="2" timestamp="2015-11-27T21:13:32Z">
    <tag k="website" v="maps.me"/>
  </node>
  )";

  XMLFeature const baseOsmFeature = XMLFeature::FromOSM(kOsmFeature).front();

  {
    XMLFeature noAnyTags = baseOsmFeature;
    noAnyTags.ApplyPatch(XMLFeature(kPatch));
    TEST(noAnyTags.HasKey("website"), ());
  }

  {
    XMLFeature hasMainTag = baseOsmFeature;
    hasMainTag.SetTagValue("website", "mapswith.me");
    hasMainTag.ApplyPatch(XMLFeature(kPatch));
    TEST_EQUAL(hasMainTag.GetTagValue("website"), "maps.me", ());
    size_t tagsCount = 0;
    hasMainTag.ForEachTag([&tagsCount](std::string const &, std::string const &) { ++tagsCount; });
    TEST_EQUAL(2, tagsCount, ("website should be replaced, not duplicated."));
  }

  {
    XMLFeature hasAltTag = baseOsmFeature;
    hasAltTag.SetTagValue("contact:website", "mapswith.me");
    hasAltTag.ApplyPatch(XMLFeature(kPatch));
    TEST(!hasAltTag.HasTag("website"), ("Existing alt tag should be used."));
    TEST_EQUAL(hasAltTag.GetTagValue("contact:website"), "maps.me", ());
  }

  {
    XMLFeature hasAltTag = baseOsmFeature;
    hasAltTag.SetTagValue("url", "mapswithme.com");
    hasAltTag.ApplyPatch(XMLFeature(kPatch));
    TEST(!hasAltTag.HasTag("website"), ("Existing alt tag should be used."));
    TEST_EQUAL(hasAltTag.GetTagValue("url"), "maps.me", ());
  }

  {
    XMLFeature hasTwoAltTags = baseOsmFeature;
    hasTwoAltTags.SetTagValue("contact:website", "mapswith.me");
    hasTwoAltTags.SetTagValue("url", "mapswithme.com");
    hasTwoAltTags.ApplyPatch(XMLFeature(kPatch));
    TEST(!hasTwoAltTags.HasTag("website"), ("Existing alt tag should be used."));
    TEST_EQUAL(hasTwoAltTags.GetTagValue("contact:website"), "maps.me", ());
    TEST_EQUAL(hasTwoAltTags.GetTagValue("url"), "mapswithme.com", ());
  }

  {
    XMLFeature hasMainAndAltTag = baseOsmFeature;
    hasMainAndAltTag.SetTagValue("website", "osmrulezz.com");
    hasMainAndAltTag.SetTagValue("url", "mapswithme.com");
    hasMainAndAltTag.ApplyPatch(XMLFeature(kPatch));
    TEST_EQUAL(hasMainAndAltTag.GetTagValue("website"), "maps.me", ());
    TEST_EQUAL(hasMainAndAltTag.GetTagValue("url"), "mapswithme.com", ());
  }
}

UNIT_TEST(XMLFeature_FromXMLAndBackToXML)
{
  classificator::Load();

  std::string const xmlNoTypeStr = R"(<?xml version="1.0"?>
  <node lat="55.7978998" lon="37.474528" timestamp="2015-11-27T21:13:32Z">
  <tag k="name" v="Gorki Park" />
  <tag k="name:en" v="Gorki Park" />
  <tag k="name:ru" v="Парк Горького" />
  <tag k="addr:housenumber" v="10" />
  </node>
  )";

  char const kTimestamp[] = "2015-11-27T21:13:32Z";

  editor::XMLFeature xmlNoType(xmlNoTypeStr);
  editor::XMLFeature xmlWithType = xmlNoType;
  xmlWithType.SetTagValue("amenity", "atm");

  osm::EditableMapObject emo;
  editor::FromXML(xmlWithType, emo);
  auto fromFtWithType = editor::ToXML(emo, true);
  fromFtWithType.SetAttribute("timestamp", kTimestamp);
  TEST_EQUAL(fromFtWithType, xmlWithType, ());

  auto fromFtWithoutType = editor::ToXML(emo, false);
  fromFtWithoutType.SetAttribute("timestamp", kTimestamp);
  TEST_EQUAL(fromFtWithoutType, xmlNoType, ());
}

UNIT_TEST(XMLFeature_AmenityRecyclingFromAndToXml)
{
  classificator::Load();
  {
    std::string const recyclingCentreStr = R"(<?xml version="1.0"?>
    <node lat="55.8047445" lon="37.5865532" timestamp="2018-07-11T13:24:41Z">
    <tag k="amenity" v="recycling" />
    <tag k="recycling_type" v="centre" />
    </node>
    )";

    char const kTimestamp[] = "2018-07-11T13:24:41Z";

    editor::XMLFeature xmlFeature(recyclingCentreStr);

    osm::EditableMapObject emo;
    editor::FromXML(xmlFeature, emo);

    auto const th = emo.GetTypes();
    TEST_EQUAL(th.Size(), 1, ());
    TEST_EQUAL(th.front(), classif().GetTypeByPath({"amenity", "recycling", "centre"}), ());

    auto convertedFt = editor::ToXML(emo, true);
    convertedFt.SetAttribute("timestamp", kTimestamp);
    TEST_EQUAL(xmlFeature, convertedFt, ());
  }
  {
    std::string const recyclingContainerStr = R"(<?xml version="1.0"?>
    <node lat="55.8047445" lon="37.5865532" timestamp="2018-07-11T13:24:41Z">
    <tag k="amenity" v="recycling" />
    <tag k="recycling_type" v="container" />
    </node>
    )";

    char const kTimestamp[] = "2018-07-11T13:24:41Z";

    editor::XMLFeature xmlFeature(recyclingContainerStr);

    osm::EditableMapObject emo;
    editor::FromXML(xmlFeature, emo);

    auto const th = emo.GetTypes();
    TEST_EQUAL(th.Size(), 1, ());
    TEST_EQUAL(th.front(), classif().GetTypeByPath({"amenity", "recycling", "container"}), ());

    auto convertedFt = editor::ToXML(emo, true);
    convertedFt.SetAttribute("timestamp", kTimestamp);
    TEST_EQUAL(xmlFeature, convertedFt, ());
  }
  /*
  {
    std::string const recyclingStr = R"(<?xml version="1.0"?>
    <node lat="55.8047445" lon="37.5865532" timestamp="2018-07-11T13:24:41Z">
    <tag k="amenity" v="recycling" />
    </node>
    )";

    editor::XMLFeature xmlFeature(recyclingStr);

    osm::EditableMapObject emo;
    editor::FromXML(xmlFeature, emo);

    auto const th = emo.GetTypes();
    TEST_EQUAL(th.Size(), 1, ());
    TEST_EQUAL(*th.begin(), classif().GetTypeByPath({"amenity", "recycling"}), ());

    auto convertedFt = editor::ToXML(emo, true);

    // We save recycling container with "recycling_type"="container" tag.
    TEST(convertedFt.HasTag("recycling_type"), ());
    TEST_EQUAL(convertedFt.GetTagValue("recycling_type"), "container", ());

    TEST(convertedFt.HasTag("amenity"), ());
    TEST_EQUAL(convertedFt.GetTagValue("amenity"), "recycling", ());
  }
  */
}

UNIT_TEST(XMLFeature_Diet)
{
  XMLFeature ft(XMLFeature::Type::Node);
  TEST(ft.GetCuisine().empty(), ());

  ft.SetCuisine("vegan;vegetarian");
  TEST_EQUAL(ft.GetCuisine(), "vegan;vegetarian", ());

  ft.SetCuisine("vegan;pasta;vegetarian");
  TEST_EQUAL(ft.GetCuisine(), "pasta;vegan;vegetarian", ());

  ft.SetCuisine("vegetarian");
  TEST_EQUAL(ft.GetCuisine(), "vegetarian", ());

  ft.SetCuisine("vegan");
  TEST_EQUAL(ft.GetCuisine(), "vegan", ());

  ft.SetCuisine("");
  TEST_EQUAL(ft.GetCuisine(), "", ());
}

UNIT_TEST(XMLFeature_SocialContactsProcessing)
{
  {
    std::string const nightclubStr = R"(<?xml version="1.0"?>
    <node lat="50.4082862" lon="30.5130017" timestamp="2022-02-24T05:07:00Z">
    <tag k="amenity" v="nightclub" />
    <tag k="name" v="Stereo Plaza" />
    <tag k="contact:facebook" v="http://www.facebook.com/pages/Stereo-Plaza/118100041593935" />
    <tag k="contact:instagram" v="https://www.instagram.com/p/CSy87IhMhfm/" />
    <tag k="contact:line" v="liff.line.me/1645278921-kWRPP32q/?accountId=673watcr" />
    </node>
    )";

    editor::XMLFeature xmlFeature(nightclubStr);

    osm::EditableMapObject emo;
    editor::FromXML(xmlFeature, emo);

    auto convertedFt = editor::ToXML(emo, true);

    TEST(convertedFt.HasTag("contact:facebook"), ());
    TEST_EQUAL(convertedFt.GetTagValue("contact:facebook"), "https://facebook.com/pages/Stereo-Plaza/118100041593935",
               ());

    TEST(convertedFt.HasTag("contact:instagram"), ());
    TEST_EQUAL(convertedFt.GetTagValue("contact:instagram"), "https://instagram.com/p/CSy87IhMhfm", ());

    TEST(convertedFt.HasTag("contact:line"), ());
    TEST_EQUAL(convertedFt.GetTagValue("contact:line"), "https://liff.line.me/1645278921-kWRPP32q/?accountId=673watcr",
               ());
  }
}

UNIT_TEST(XMLFeature_SocialContactsProcessing_clean)
{
  {
    std::string const nightclubStr = R"(<?xml version="1.0"?>
    <node lat="40.82862" lon="20.30017" timestamp="2022-02-24T05:07:00Z">
    <tag k="amenity" v="bar" />
    <tag k="name" v="Irish Pub" />
    <tag k="contact:facebook" v="https://www.facebook.com/PierreCardinPeru.oficial/" />
    <tag k="contact:instagram" v="https://www.instagram.com/fraback.genusswelt/" />
    <tag k="contact:line" v="https://line.me/R/ti/p/%40015qevdv" />
    </node>
    )";

    editor::XMLFeature xmlFeature(nightclubStr);

    osm::EditableMapObject emo;
    editor::FromXML(xmlFeature, emo);

    auto convertedFt = editor::ToXML(emo, true);

    TEST(convertedFt.HasTag("contact:facebook"), ());
    TEST_EQUAL(convertedFt.GetTagValue("contact:facebook"), "PierreCardinPeru.oficial", ());

    TEST(convertedFt.HasTag("contact:instagram"), ());
    TEST_EQUAL(convertedFt.GetTagValue("contact:instagram"), "fraback.genusswelt", ());

    TEST(convertedFt.HasTag("contact:line"), ());
    TEST_EQUAL(convertedFt.GetTagValue("contact:line"), "015qevdv", ());
  }
}
