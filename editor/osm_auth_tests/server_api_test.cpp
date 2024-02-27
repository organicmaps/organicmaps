#include "testing/testing.hpp"

#include "editor/server_api.hpp"

#include "geometry/mercator.hpp"

#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <string>
#include <random>

#include <pugixml.hpp>

using osm::ServerApi06;
using osm::OsmOAuth;
using namespace pugi;

extern char const * kValidOsmUser;
extern char const * kValidOsmPassword;

UNIT_TEST(OSM_ServerAPI_TestUserExists)
{
  ServerApi06 api(OsmOAuth::DevServerAuth());
  TEST(api.TestOSMUser(kValidOsmUser), ());
  TEST(!api.TestOSMUser("donotregisterthisuser"), ());
}

namespace
{
ServerApi06 CreateAPI()
{
  OsmOAuth auth = OsmOAuth::DevServerAuth();
  bool result;
  TEST_NO_THROW(result = auth.AuthorizePassword(kValidOsmUser, kValidOsmPassword), ());
  TEST_EQUAL(result, true, ("Invalid login or password?"));
  TEST(auth.IsAuthorized(), ("OSM authorization"));
  ServerApi06 api(auth);
  // Test user preferences reading along the way.
  osm::UserPreferences prefs;
  TEST_NO_THROW(prefs = api.GetUserPreferences(), ());
  TEST_EQUAL(prefs.m_displayName, kValidOsmUser, ("User display name"));
  TEST_EQUAL(prefs.m_id, 14235, ("User id"));
  return api;
}

// Returns random coordinate to avoid races when several workers run tests at the same time.
ms::LatLon RandomCoordinate()
{
  std::random_device rd;
  return ms::LatLon(std::uniform_real_distribution<>{-89., 89.}(rd),
                    std::uniform_real_distribution<>{-179., 179.}(rd));
}

} // namespace

void DeleteOSMNodeIfExists(ServerApi06 const & api, uint64_t changeSetId, ms::LatLon const & ll)
{
  // Delete all test nodes left on the server (if any).
  auto const response = api.GetXmlFeaturesAtLatLon(ll, 1.0);
  TEST_EQUAL(response.first, OsmOAuth::HTTP::OK, ());
  xml_document reply;
  reply.load_string(response.second.c_str());
  // Response can be empty, and it's ok.
  for (pugi::xml_node node : reply.child("osm").children("node"))
  {
    node.attribute("changeset") = changeSetId;
    node.remove_child("tag");
    TEST_NO_THROW(api.DeleteElement(editor::XMLFeature(node)), ());
  }
}

UNIT_TEST(OSM_ServerAPI_ChangesetAndNode)
{
  ms::LatLon const kOriginalLocation = RandomCoordinate();
  ms::LatLon const kModifiedLocation = RandomCoordinate();

  using editor::XMLFeature;
  XMLFeature node(XMLFeature::Type::Node);

  ServerApi06 const api = CreateAPI();
  uint64_t changeSetId = api.CreateChangeSet({{"created_by", "OMaps Unit Test"},
                                              {"comment", "For test purposes only."}});
  auto const changesetCloser = [&]() { api.CloseChangeSet(changeSetId); };

  {
    SCOPE_GUARD(guard, changesetCloser);

    // Sometimes network can unexpectedly fail (or test exception can be raised), so do some cleanup before unit tests.
    DeleteOSMNodeIfExists(api, changeSetId, kOriginalLocation);
    DeleteOSMNodeIfExists(api, changeSetId, kModifiedLocation);

    node.SetCenter(kOriginalLocation);
    node.SetAttribute("changeset", strings::to_string(changeSetId));
    node.SetAttribute("version", "1");
    node.SetTagValue("testkey", "firstnode");

    // Pushes node to OSM server and automatically sets node id.
    api.CreateElementAndSetAttributes(node);
    TEST(!node.GetAttribute("id").empty(), ());

    // Change node's coordinates and tags.
    node.SetCenter(kModifiedLocation);
    node.SetTagValue("testkey", "secondnode");
    api.ModifyElementAndSetVersion(node);
    // After modification, node version increases in ModifyElement.
    TEST_EQUAL(node.GetAttribute("version"), "2", ());

    // All tags must be specified, because there is no merging of old and new tags.
    api.UpdateChangeSet(changeSetId, {{"created_by", "OMaps Unit Test"},
                                      {"comment", "For test purposes only (updated)."}});

    // To retrieve created node, changeset should be closed first.
    // It is done here via Scope Guard.
  }

  auto const response = api.GetXmlFeaturesAtLatLon(kModifiedLocation, 1.0);
  TEST_EQUAL(response.first, OsmOAuth::HTTP::OK, ());
  auto const features = XMLFeature::FromOSM(response.second);
  TEST_EQUAL(1, features.size(), ());
  TEST_EQUAL(node.GetAttribute("id"), features[0].GetAttribute("id"), ());

  // Cleanup - delete unit test node from the server.
  changeSetId = api.CreateChangeSet({{"created_by", "OMaps Unit Test"},
                                     {"comment", "For test purposes only."}});
  SCOPE_GUARD(guard, changesetCloser);
  // New changeset has new id.
  node.SetAttribute("changeset", strings::to_string(changeSetId));
  TEST_NO_THROW(api.DeleteElement(node), ());
}

UNIT_TEST(OSM_ServerAPI_Notes)
{
  ms::LatLon const pos = RandomCoordinate();
  ServerApi06 const api = CreateAPI();
  uint64_t id;
  TEST_NO_THROW(id = api.CreateNote(pos, "A test note"), ("Creating a note"));
  TEST_GREATER(id, 0, ("Note id should be a positive integer"));
  TEST_NO_THROW(api.CloseNote(id), ("Closing a note"));
}
