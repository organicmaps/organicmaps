#include "testing/testing.hpp"

#include "editor/server_api.hpp"

#include "geometry/mercator.hpp"

#include "3party/pugixml/src/pugixml.hpp"

using osm::ServerApi06;
using osm::OsmOAuth;
using namespace pugi;

constexpr char const * kValidOsmUser = "MapsMeTestUser";
constexpr char const * kInvalidOsmUser = "qwesdxzcgretwr";
constexpr char const * kValidOsmPassword = "12345678";

UNIT_TEST(OSM_ServerAPI_TestUserExists)
{
  ServerApi06 api(OsmOAuth::DevServerAuth());
  TEST_EQUAL(OsmOAuth::ResponseCode::OK, api.TestUserExists(kValidOsmUser), ());
  TEST_EQUAL(OsmOAuth::ResponseCode::NotFound, api.TestUserExists(kInvalidOsmUser), ());
}

namespace
{
ServerApi06 CreateAPI()
{
  OsmOAuth auth = OsmOAuth::DevServerAuth();
  OsmOAuth::AuthResult result = auth.AuthorizePassword(kValidOsmUser, kValidOsmPassword);
  TEST_EQUAL(result, OsmOAuth::AuthResult::OK, ());
  TEST(auth.IsAuthorized(), ("OSM authorization"));
  ServerApi06 api(auth);
  return api;
}

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
  ServerApi06 api = CreateAPI();

  uint64_t changeSetId;
  TEST(api.CreateChangeSet({{"created_by", "MAPS.ME Unit Test"}, {"comment", "For test purposes only."}}, changeSetId), ());

  xml_document node;
  GenerateNodeXml(11.11, 12.12, {{"testkey", "firstnode"}}, node);
  TEST(SetAttributeForOsmNode(node, "changeset", changeSetId), ());

  uint64_t nodeId;
  TEST(api.CreateNode(XmlToString(node), nodeId), ());
  TEST(SetAttributeForOsmNode(node, "id", nodeId), ());

  TEST(SetAttributeForOsmNode(node, "lat", 10.10), ());
  TEST(api.ModifyNode(XmlToString(node), nodeId), ());
  // After modification, node version has increased.
  TEST(SetAttributeForOsmNode(node, "version", 2), ());

  // To retrieve created node, changeset should be closed first.
  TEST(api.CloseChangeSet(changeSetId), ());

  TEST(api.CreateChangeSet({{"created_by", "MAPS.ME Unit Test"}, {"comment", "For test purposes only."}}, changeSetId), ());
  // New changeset has new id.
  TEST(SetAttributeForOsmNode(node, "changeset", changeSetId), ());

  auto const response = api.GetXmlFeaturesAtLatLon(node.child("osm").child("node").attribute("lat").as_double(),
                                               node.child("osm").child("node").attribute("lon").as_double());
  TEST_EQUAL(response.first, OsmOAuth::ResponseCode::OK, ());
  xml_document reply;
  reply.load_string(response.second.c_str());
  TEST_EQUAL(nodeId, reply.child("osm").child("node").attribute("id").as_ullong(), ());

  TEST(ServerApi06::DeleteResult::ESuccessfullyDeleted == api.DeleteNode(XmlToString(node), nodeId), ());

  TEST(api.CloseChangeSet(changeSetId), ());
}
