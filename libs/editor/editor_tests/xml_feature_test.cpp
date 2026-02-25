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

UNIT_TEST(XMLFeature_FromXMLAndBackToXML)
{
  classificator::Load();

  /// @todo The order of "tag" values is important for the final TEST_EQUAL comparison.
  /// Make it stable?
  std::string_view const data = R"(<?xml version="1.0"?>
  <node lat="55.7978998" lon="37.474528" timestamp="2015-11-27T21:13:32Z">
    <tag k="name" v="Gorki Park" />
    <tag k="addr:housenumber" v="10" />
    <tag k="amenity" v="studio" />
    <journal version="1.0">
      <entry type="TagModification" timestamp="2026-02-25T17:15:00Z">
        <data key="name" old_value="" new_value="Gorki Park" />
      </entry>
      <entry type="TagModification" timestamp="2026-02-25T17:15:00Z">
        <data key="addr:housenumber" old_value="" new_value="10" />
      </entry>
    </journal>
    <journalHistory version="1.0">
      <entry type="ObjectCreated" timestamp="2015-11-27T21:13:32Z">
        <data type="amenity-studio" geomType="Point" lat="55.7978998" lon="37.474528" />
      </entry>
    </journalHistory>
  </node>
  )";

  char const kTimestamp[] = "2015-11-27T21:13:32Z";

  editor::XMLFeature xmlFeature(data);

  osm::EditableMapObject emo;
  osm::EditJournal journal = xmlFeature.GetEditJournal();
  emo.ApplyEditsFromJournal(journal);
  emo.SetJournal(std::move(journal));

  auto fromEmo = editor::ToXML(emo, true);
  fromEmo.SetEditJournal(emo.GetJournal());
  fromEmo.SetAttribute("timestamp", kTimestamp);

  TEST_EQUAL(fromEmo, xmlFeature, ());
}

UNIT_TEST(XMLFeature_AmenityRecyclingFromAndToXml)
{
  classificator::Load();
  {
    std::string_view const recyclingCentreStr = R"(<?xml version="1.0"?>
    <node lat="55.8047445" lon="37.5865532" timestamp="2018-07-11T13:24:41Z">
    <tag k="amenity" v="recycling" />
    <tag k="recycling_type" v="centre" />
    </node>
    )";

    char const kTimestamp[] = "2018-07-11T13:24:41Z";

    editor::XMLFeature xmlFeature(recyclingCentreStr);

    osm::EditableMapObject emo;
    emo.SetType(classif().GetTypeByPath({"amenity", "recycling", "centre"}));
    emo.SetMercator(mercator::FromLatLon(55.8047445, 37.5865532));

    auto convertedFt = editor::ToXML(emo, true);
    convertedFt.SetAttribute("timestamp", kTimestamp);
    TEST_EQUAL(xmlFeature, convertedFt, ());
  }
  {
    std::string_view const recyclingContainerStr = R"(<?xml version="1.0"?>
    <node lat="55.8047445" lon="37.5865532" timestamp="2018-07-11T13:24:41Z">
    <tag k="amenity" v="recycling" />
    <tag k="recycling_type" v="container" />
    </node>
    )";

    char const kTimestamp[] = "2018-07-11T13:24:41Z";

    editor::XMLFeature xmlFeature(recyclingContainerStr);

    osm::EditableMapObject emo;
    emo.SetType(classif().GetTypeByPath({"amenity", "recycling", "container"}));
    emo.SetMercator(mercator::FromLatLon(55.8047445, 37.5865532));

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
    emo.SetType(classif().GetTypeByPath({"amenity", "recycling"}));
    emo.SetMercator(mercator::FromLatLon(55.8047445, 37.5865532));

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
  classificator::Load();

  {
    osm::EditableMapObject emo;
    emo.SetType(classif().GetTypeByPath({"amenity", "nightclub"}));
    emo.SetMercator(mercator::FromLatLon(50.4082862, 30.5130017));
    emo.SetName("Stereo Plaza", StringUtf8Multilang::kDefaultCode);

    emo.UpdateMetadataValue("contact:facebook", "http://www.facebook.com/pages/Stereo-Plaza/118100041593935");
    emo.UpdateMetadataValue("contact:instagram", "https://www.instagram.com/p/CSy87IhMhfm/");
    emo.UpdateMetadataValue("contact:line", "liff.line.me/1645278921-kWRPP32q/?accountId=673watcr");

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

  {
    osm::EditableMapObject emo;
    emo.SetType(classif().GetTypeByPath({"amenity", "bar"}));
    emo.SetMercator(mercator::FromLatLon(40.82862, 20.30017));
    emo.SetName("Irish Pub", StringUtf8Multilang::kDefaultCode);

    emo.UpdateMetadataValue("contact:facebook", "https://www.facebook.com/PierreCardinPeru.oficial/");
    emo.UpdateMetadataValue("contact:instagram", "https://www.instagram.com/fraback.genusswelt/");
    emo.UpdateMetadataValue("contact:line", "https://line.me/R/ti/p/%40015qevdv");

    auto convertedFt = editor::ToXML(emo, true);

    TEST(convertedFt.HasTag("contact:facebook"), ());
    TEST_EQUAL(convertedFt.GetTagValue("contact:facebook"), "PierreCardinPeru.oficial", ());

    TEST(convertedFt.HasTag("contact:instagram"), ());
    TEST_EQUAL(convertedFt.GetTagValue("contact:instagram"), "fraback.genusswelt", ());

    TEST(convertedFt.HasTag("contact:line"), ());
    TEST_EQUAL(convertedFt.GetTagValue("contact:line"), "015qevdv", ());
  }
}
