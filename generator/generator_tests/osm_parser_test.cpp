#include "../../testing/testing.hpp"

#include "../osm_xml_parser.hpp"
#include "../borders_generator.hpp"

#include "../../coding/reader.hpp"
#include "../../coding/parse_xml.hpp"
#include "../../coding/file_reader.hpp"
#include "../../coding/file_writer.hpp"

using namespace osm;

static char const gOsmXml[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<osm version=\"0.6\" generator=\"OpenStreetMap server\">"
"  <node id=\"1\" lat=\"0.0\" lon=\"0.0\" visible=\"true\"/>"
"  <node id=\"2\" lat=\"20.0\" lon=\"0.0\"/>"
"  <node id=\"3\" lat=\"20.0\" lon=\"20.0\"/>"
"  <node id=\"4\" lat=\"0.0\" lon=\"20.0\"/>"
"  <node id=\"7\" lat=\"5.0\" lon=\"10.0\"/>"
"  <node id=\"8\" lat=\"10.0\" lon=\"10.0\"/>"
"  <node id=\"9\" lat=\"10.0\" lon=\"15.0\"/>"
"  <node id=\"10\" lat=\"5.555555\" lon=\"4.222222\">"
"    <tag k=\"name\" v=\"Andorra la Vella\"/>"
"    <tag k=\"capital\" v=\"yes\"/>"
"  </node>"
"  <node id=\"11\" lat=\"105.0\" lon=\"110.0\"/>"
"  <node id=\"12\" lat=\"110.0\" lon=\"110.0\"/>"
"  <node id=\"13\" lat=\"110.0\" lon=\"115.0\"/>"
"  <way id=\"100\" visible=\"true\">"
"    <nd ref=\"1\"/>"
"    <nd ref=\"2\"/>"
"    <nd ref=\"3\"/>"
"    <tag k=\"boundary\" v=\"administrative\"/>"
"    <tag k=\"admin_level\" v=\"2\"/>"
"  </way>"
"  <way id=\"101\" visible=\"true\">"
"    <nd ref=\"1\"/>"
"    <nd ref=\"4\"/>"
"    <nd ref=\"3\"/>"
"    <tag k=\"boundary\" v=\"administrative\"/>"
"    <tag k=\"admin_level\" v=\"2\"/>"
"  </way>"
"  <way id=\"102\" visible=\"true\">"
"    <nd ref=\"11\"/>"
"    <nd ref=\"12\"/>"
"    <nd ref=\"13\"/>"
"    <nd ref=\"11\"/>"
"    <tag k=\"boundary\" v=\"administrative\"/>"
"    <tag k=\"admin_level\" v=\"2\"/>"
"  </way>"
"  <way id=\"121\" visible=\"true\">"
"    <nd ref=\"7\"/>"
"    <nd ref=\"8\"/>"
"    <nd ref=\"9\"/>"
"    <nd ref=\"7\"/>"
"    <tag k=\"boundary\" v=\"administrative\"/>"
"    <tag k=\"admin_level\" v=\"2\"/>"
"  </way>"
"  <relation id=\"444\" visible=\"true\">"
"    <member type=\"way\" ref=\"121\" role=\"outer\"/>"
"    <tag k=\"admin_level\" v=\"4\"/>"
"    <tag k=\"boundary\" v=\"administrative\"/>"
"    <tag k=\"name\" v=\"Some Region\"/>"
"    <tag k=\"type\" v=\"boundary\"/>"
"  </relation>"
"  <relation id=\"555\" visible=\"true\">"
"    <member type=\"node\" ref=\"10\" role=\"capital\"/>"
"    <member type=\"way\" ref=\"100\" role=\"\"/>"
"    <member type=\"way\" ref=\"101\" role=\"outer\"/>"
"    <member type=\"way\" ref=\"102\" role=\"\"/>"
"    <member type=\"way\" ref=\"121\" role=\"inner\"/>"
"    <member type=\"relation\" ref=\"444\" role=\"subarea\"/>"
"    <tag k=\"admin_level\" v=\"2\"/>"
"    <tag k=\"boundary\" v=\"administrative\"/>"
"    <tag k=\"ISO3166-1\" v=\"ad\"/>"
"    <tag k=\"name\" v=\"Andorra\"/>"
"    <tag k=\"name:en\" v=\"Andorra\"/>"
"    <tag k=\"type\" v=\"boundary\"/>"
"  </relation>"
"</osm>";

#define TEST_EXCEPTION(exception, expression) do { \
  bool gotException = false;  \
  try { expression; } \
  catch (exception const &) { gotException = true; } \
  TEST(gotException, ("Exception should be thrown:", #exception)); \
  } while(0)


struct PointsTester
{
  list<OsmId> m_controlPoints;
  PointsTester()
  {
    m_controlPoints.push_back(1);
    m_controlPoints.push_back(2);
    m_controlPoints.push_back(3);
    m_controlPoints.push_back(4);
    m_controlPoints.push_back(1);
  }
  void operator()(OsmId const & ptId)
  {
    TEST(!m_controlPoints.empty(), ());
    TEST_EQUAL(m_controlPoints.front(), ptId, ());
    m_controlPoints.pop_front();
  }

  bool IsOk() const
  {
    return m_controlPoints.empty();
  }
};

UNIT_TEST(OsmRawData_SmokeTest)
{
  OsmRawData osmData;

  {
    // -1 to avoid finishing zero at the end of the string
    MemReader xmlBlock(gOsmXml, ARRAY_SIZE(gOsmXml) - 1);
    ReaderSource<MemReader> source(xmlBlock);

    OsmXmlParser parser(osmData);
    TEST(ParseXML(source, parser), ("Invalid XML"));
  }

  string outTagValue;

  TEST_EXCEPTION(OsmRawData::OsmInvalidIdException, OsmNode node = osmData.NodeById(98764));

  OsmNode node = osmData.NodeById(9);
  TEST_EQUAL(node.m_lat, 10.0, ());
  TEST_EQUAL(node.m_lon, 15.0, ());

  TEST_EXCEPTION(OsmRawData::OsmInvalidIdException, OsmWay way = osmData.WayById(635794));

  OsmWay way = osmData.WayById(100);
  TEST_EQUAL(way.PointsCount(), 3, ());
  TEST(!way.TagValueByKey("invalid_tag", outTagValue), ());
  TEST(way.TagValueByKey("boundary", outTagValue), ());
  TEST_EQUAL(outTagValue, "administrative", ());
  TEST(way.TagValueByKey("admin_level", outTagValue), ());
  TEST_EQUAL(outTagValue, "2", ());

  OsmWay way2 = osmData.WayById(101);
  TEST(way.MergeWith(way2), ());
  TEST_EQUAL(way.PointsCount(), 5, ());
  PointsTester tester;
  way.ForEachPoint(tester);
  TEST(tester.IsOk(), ());
  TEST(way.IsClosed(), ());

  TEST_EXCEPTION(OsmRawData::OsmInvalidIdException, OsmRelation relation = osmData.RelationById(64342));

  OsmRelation rel1 = osmData.RelationById(444);
  TEST(rel1.TagValueByKey("admin_level", outTagValue), ());
  TEST_EQUAL(outTagValue, "4", ());

  OsmIds relations = osmData.RelationsByKey("invalid_tag_key");
  TEST(relations.empty(), ());
  relations = osmData.RelationsByKey("ISO3166-1");
  TEST_EQUAL(relations.size(), 1, ());
  TEST_EQUAL(relations[0], 555, ());

  OsmRelation rel2 = osmData.RelationById(relations[0]);
  OsmIds members = rel2.MembersByTypeAndRole("way", "");
  TEST_EQUAL(members.size(), 2, ());
  TEST_EQUAL(members[0], 100, ());
  members = rel2.MembersByTypeAndRole("way", "outer");
  TEST_EQUAL(members.size(), 1, ());
  TEST_EQUAL(members[0], 101, ());
  members = rel2.MembersByTypeAndRole("relation", "invalid_role");
  TEST(members.empty(), ());

  relations.clear();

  relations = osmData.RelationsByTag(OsmTag("boundary_invalid", "administrative"));
  TEST(relations.empty(), ());
  relations = osmData.RelationsByTag(OsmTag("boundary", "administrative"));
  TEST_EQUAL(relations.size(), 2, ());
}
