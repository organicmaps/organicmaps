#include "testing/testing.hpp"

#include "editor/server_api.hpp"

#include "geometry/mercator.hpp"

#include "std/cstring.hpp"

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
  OsmOAuth::AuthResult const result = auth.AuthorizePassword(kValidOsmUser, kValidOsmPassword);
  TEST_EQUAL(result, OsmOAuth::AuthResult::OK, ());
  TEST(auth.IsAuthorized(), ("OSM authorization"));
  ServerApi06 api(auth);
  // Test user preferences reading along the way.
  osm::UserPreferences prefs;
  OsmOAuth::ResponseCode const code = api.GetUserPreferences(prefs);
  TEST_EQUAL(code, OsmOAuth::ResponseCode::OK, ("Request user preferences"));
  TEST_EQUAL(prefs.m_displayName, kValidOsmUser, ("User display name"));
  TEST_EQUAL(prefs.m_id, 3500, ("User id"));
  return api;
}

// id attribute is set to -1.
// version attribute is set to 1.
void GenerateNodeXml(ms::LatLon const & ll, ServerApi06::TKeyValueTags const & tags, xml_document & outNode)
{
  outNode.reset();
  xml_node node = outNode.append_child("osm").append_child("node");
  node.append_attribute("id") = -1;
  node.append_attribute("version") = 1;
  node.append_attribute("lat") = ll.lat;
  node.append_attribute("lon") = ll.lon;
  for (auto const & kv : tags)
  {
    xml_node tag = node.append_child("tag");
    tag.append_attribute("k").set_value(kv.first.c_str());
    tag.append_attribute("v").set_value(kv.second.c_str());
  }
}

string XmlToString(xml_node const & node)
{
  ostringstream stream;
  node.print(stream);
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

void DeleteOSMNodeIfExists(ServerApi06 const & api, uint64_t changeSetId, ms::LatLon const & ll)
{
  // Delete all test nodes left on the server (if any).
  auto const response = api.GetXmlFeaturesAtLatLon(ll);
  TEST_EQUAL(response.first, OsmOAuth::ResponseCode::OK, ());
  xml_document reply;
  reply.load_string(response.second.c_str());
  // Response can be empty, and it's ok.
  for (pugi::xml_node node : reply.child("osm").children("node"))
  {
    node.attribute("changeset") = changeSetId;
    node.remove_child("tag");
    TEST(ServerApi06::DeleteResult::ESuccessfullyDeleted == api.DeleteNode(
           "<osm>\n" + XmlToString(node) + "</osm>", node.attribute("id").as_ullong()), ());
  }
}

UNIT_TEST(OSM_ServerAPI_ChangesetActions)
{
  ServerApi06 const api = CreateAPI();

  uint64_t changeSetId;
  TEST(api.CreateChangeSet({{"created_by", "MAPS.ME Unit Test"}, {"comment", "For test purposes only."}}, changeSetId), ());

  ms::LatLon const originalLocation(11.11, 12.12);
  // Sometimes network can unexpectedly fail (or test exception can be raised), so do some cleanup before unit tests.
  DeleteOSMNodeIfExists(api, changeSetId, originalLocation);
  ms::LatLon const modifiedLocation(10.10, 12.12);
  DeleteOSMNodeIfExists(api, changeSetId, modifiedLocation);

  xml_document node;
  GenerateNodeXml(originalLocation, {{"testkey", "firstnode"}}, node);
  TEST(SetAttributeForOsmNode(node, "changeset", changeSetId), ());

  uint64_t nodeId;
  TEST(api.CreateNode(XmlToString(node), nodeId), ());
  TEST(SetAttributeForOsmNode(node, "id", nodeId), ());

  TEST(SetAttributeForOsmNode(node, "lat", modifiedLocation.lat), ());
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
