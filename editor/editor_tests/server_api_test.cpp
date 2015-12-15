#include "testing/testing.hpp"

#include "editor/server_api.hpp"

#include "geometry/mercator.hpp"

#include "3party/pugixml/src/pugixml.hpp"

using osm::ServerApi06;
using namespace pugi;

constexpr char const * kOsmDevServer = "http://master.apis.dev.openstreetmap.org";
constexpr char const * kValidOsmUser = "MapsMeTestUser";
constexpr char const * kInvalidOsmUser = "qwesdxzcgretwr";
ServerApi06 const kApi(kValidOsmUser, "12345678", kOsmDevServer);

UNIT_TEST(OSM_ServerAPI_CheckUserAndPassword)
{
  TEST(kApi.CheckUserAndPassword(), ());

  my::LogLevelSuppressor s;
  TEST(!ServerApi06(kInvalidOsmUser, "3345dfce2", kOsmDevServer).CheckUserAndPassword(), ());
}

UNIT_TEST(OSM_ServerAPI_HttpCodeForUrl)
{
  TEST_EQUAL(200, ServerApi06::HttpCodeForUrl(string(kOsmDevServer) + "/user/" + kValidOsmUser), ());
  TEST_EQUAL(404, ServerApi06::HttpCodeForUrl(string(kOsmDevServer) + "/user/" + kInvalidOsmUser), ());
}

namespace
{
// id attribute is set to -1.
// version attribute is set to 1.
void GenerateNodeXml(double lat, double lon, ServerApi06::TKeyValueTags const & tags, xml_document & outNode)
{
  outNode.reset();
  xml_node node = outNode.append_child("osm").append_child("node");
  node.append_attribute("id") = -1;
  node.append_attribute("version") = 1;
  node.append_attribute("lat") = lat;
  node.append_attribute("lon") = lon;
  for (auto const & kv : tags)
  {
    xml_node tag = node.append_child("tag");
    tag.append_attribute("k").set_value(kv.first.c_str());
    tag.append_attribute("v").set_value(kv.second.c_str());
  }
}

string XmlToString(xml_document const & doc)
{
  ostringstream stream;
  doc.print(stream);
  return stream.str();
}

// Replaces attribute for <node> tag (creates attribute if it's not present).
template <class TNewValue>
bool SetAttributeForOsmNode(xml_document & doc, char const * attribute, TNewValue const & v)
{
  xml_node node = doc.find_node([](xml_node const & n)
  {
      return 0 == strncmp(n.name(), "node", sizeof("node"));
  });
  if (node.empty())
    return false;
  if (node.attribute(attribute).empty())
    node.append_attribute(attribute) = v;
  else
    node.attribute(attribute) = v;
  return true;
}

} // namespace

UNIT_TEST(SetAttributeForOsmNode)
{
  xml_document doc;
  doc.append_child("osm").append_child("node");

  TEST(SetAttributeForOsmNode(doc, "Test", 123), ());
  TEST_EQUAL(123, doc.child("osm").child("node").attribute("Test").as_int(), ());

  TEST(SetAttributeForOsmNode(doc, "Test", 321), ());
  TEST_EQUAL(321, doc.child("osm").child("node").attribute("Test").as_int(), ());
}

UNIT_TEST(OSM_ServerAPI_ChangesetActions)
{
  uint64_t changeSetId;
  TEST(kApi.CreateChangeSet({{"created_by", "MAPS.ME Unit Test"}, {"comment", "For test purposes only."}}, changeSetId), ());

  xml_document node;
  GenerateNodeXml(11.11, 12.12, {{"testkey", "firstnode"}}, node);
  TEST(SetAttributeForOsmNode(node, "changeset", changeSetId), ());

  uint64_t nodeId;
  TEST(kApi.CreateNode(XmlToString(node), nodeId), ());
  TEST(SetAttributeForOsmNode(node, "id", nodeId), ());

  TEST(SetAttributeForOsmNode(node, "lat", 10.10), ());
  TEST(kApi.ModifyNode(XmlToString(node), nodeId), ());
  // After modification, node version has increased.
  TEST(SetAttributeForOsmNode(node, "version", 2), ());

  // To retrieve created node, changeset should be closed first.
  TEST(kApi.CloseChangeSet(changeSetId), ());

  TEST(kApi.CreateChangeSet({{"created_by", "MAPS.ME Unit Test"}, {"comment", "For test purposes only."}}, changeSetId), ());
  // New changeset has new id.
  TEST(SetAttributeForOsmNode(node, "changeset", changeSetId), ());

  string const serverReply = kApi.GetXmlNodeByLatLon(node.child("osm").child("node").attribute("lat").as_double(),
                                                     node.child("osm").child("node").attribute("lon").as_double());
  xml_document reply;
  reply.load_string(serverReply.c_str());
  TEST_EQUAL(nodeId, reply.child("osm").child("node").attribute("id").as_ullong(), ());

  TEST(ServerApi06::DeleteResult::ESuccessfullyDeleted == kApi.DeleteNode(XmlToString(node), nodeId), ());

  TEST(kApi.CloseChangeSet(changeSetId), ());
}
