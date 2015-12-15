#pragma once

#include "geometry/rect2d.hpp"

#include "std/map.hpp"
#include "std/string.hpp"

namespace osm
{

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

  ServerApi06(string const & user, string const & password, string const & baseUrl = "http://api.openstreetmap.org");
  /// @returns true if connection with OSM server was established, and user+password are valid.
  bool CheckUserAndPassword() const;
  /// @returns http server code for given url or negative value in case of error.
  /// This function can be used to check if user did not confirm email validation link after registration.
  /// For example, for http://www.openstreetmap.org/user/UserName 200 is returned if UserName was registered.
  static int HttpCodeForUrl(string const & url);

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
  string GetXmlFeaturesInRect(m2::RectD const & latLonRect) const;
  string GetXmlNodeByLatLon(double lat, double lon) const;

private:
  string m_user;
  string m_password;
  string m_baseOsmServerUrl;
};

} // namespace osm
