#include "testing/testing.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"

#include "base/string_utils.hpp"

#include "std/sstream.hpp"
/*
namespace
{
struct TestSetUp
{
  TestSetUp() { classificator::Load(); }
};

TestSetUp g_testSetUp;

// Sort tags and compare as strings.
void CompareFeatureXML(string const & d1, string const & d2)
{
  pugi::xml_document xml1, xml2;
  xml1.load(d1.data());
  xml2.load(d2.data());

  xml1.child("node").remove_attribute("timestamp");
  xml2.child("node").remove_attribute("timestamp");

  stringstream ss1, ss2;
  xml1.save(ss1);
  xml2.save(ss2);

  vector<string> v1(strings::SimpleTokenizer(ss1.str(), "\n"), strings::SimpleTokenizer());
  vector<string> v2(strings::SimpleTokenizer(ss2.str(), "\n"), strings::SimpleTokenizer());

  TEST(v1.size() >= 3, ());
  TEST(v2.size() >= 3, ());

  // Sort all except <xml ...>,  <node ...> and </node>
  sort(begin(v1) + 2, end(v1) - 1);
  sort(begin(v2) + 2, end(v2) - 1);

  // Format back to string to have a nice readable error message.
  auto s1 = strings::JoinStrings(v1, "\n");
  auto s2 = strings::JoinStrings(v2, "\n");

  TEST_EQUAL(s1, s2, ());
}
}  // namespace

 TODO(mgsergio): Unkomment when creation is required.
 UNIT_TEST(FeatureType_FromXMLAndBackToXML)
 {
   auto const xml = R"(<?xml version="1.0"?>
 <node
   lat="55.7978998"
   lon="37.474528"
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
   <tag
     k="amenity"
     v="atm" />
   <tag
     k="mapswithme:geom_type"
     v="GEOM_POINT" />
 </node>
 )";

   auto const feature = FeatureType::FromXML(xml);
   auto const xmlFeature = feature.ToXML();

   stringstream sstr;
   xmlFeature.Save(sstr);

   CompareFeatureXML(xml, sstr.str());
 }
*/
