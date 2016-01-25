#pragma once

#include "editor/osm_auth.hpp"

#include "geometry/latlon.hpp"
#include "geometry/rect2d.hpp"

#include "std/map.hpp"
#include "std/string.hpp"

namespace osm
{
struct UserPreferences
{
  uint64_t m_id;
  string m_displayName;
  string m_imageUrl;
  uint32_t m_changesets;
};

/// All methods here are synchronous and need wrappers for async usage.
/// TODO(AlexZ): Rewrite ServerAPI interface to accept XMLFeature.
class ServerApi06
{
public:
  // k= and v= tags used in OSM.
  using TKeyValueTags = map<string, string>;

  /// Some nodes can't be deleted if they are used in ways or relations.
  enum class DeleteResult
  {
    ESuccessfullyDeleted,
    EFailed,
    ECanNotBeDeleted
  };

  ServerApi06(OsmOAuth const & auth);
  /// This function can be used to check if user did not confirm email validation link after registration.
  /// @returns OK if user exists, NotFound if it is not, and ServerError if there is no connection.
  OsmOAuth::ResponseCode TestUserExists(string const & userName);
  /// A convenience method for UI
  OsmOAuth::ResponseCode GetUserPreferences(UserPreferences & pref) const;
  /// Please use at least created_by=* and comment=* tags.
  bool CreateChangeSet(TKeyValueTags const & kvTags, uint64_t & outChangeSetId) const;
  /// nodeXml should be wrapped into <osm> ... </osm> tags.
  bool CreateNode(string const & nodeXml, uint64_t & outCreatedNodeId) const;
  /// nodeXml should be wrapped into <osm> ... </osm> tags.
  bool ModifyNode(string const & nodeXml, uint64_t nodeId) const;
  /// nodeXml should be wrapped into <osm> ... </osm> tags.
  DeleteResult DeleteNode(string const & nodeXml, uint64_t nodeId) const;
  bool CloseChangeSet(uint64_t changesetId) const;

  /// @returns OSM xml string with features in the bounding box or empty string on error.
  OsmOAuth::Response GetXmlFeaturesInRect(m2::RectD const & latLonRect) const;
  OsmOAuth::Response GetXmlFeaturesAtLatLon(double lat, double lon) const;
  OsmOAuth::Response GetXmlFeaturesAtLatLon(ms::LatLon const & ll) const;

private:
  OsmOAuth m_auth;
};

} // namespace osm
