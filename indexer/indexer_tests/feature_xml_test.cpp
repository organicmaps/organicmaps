#include "testing/testing.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"

namespace
{
struct TestSetUp
{
  TestSetUp() { classificator::Load(); }
};

TestSetUp g_testSetUp;
}  // namespace


UNIT_TEST(FeatureType_FromXMLAndBackToXML)
{
  string const xmlNoTypeStr = R"(<?xml version="1.0"?>
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

  FeatureType ft;
  ft.FromXML(xmlWithType);
  auto fromFtWithType = ft.ToXML(true);
  fromFtWithType.SetAttribute("timestamp", kTimestamp);
  TEST_EQUAL(fromFtWithType, xmlWithType, ());

  auto fromFtWithoutType = ft.ToXML(false);
  fromFtWithoutType.SetAttribute("timestamp", kTimestamp);
  TEST_EQUAL(fromFtWithoutType, xmlNoType, ());
}
