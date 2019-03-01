#pragma once

#include "editor/osm_auth.hpp"
#include "editor/xml_feature.hpp"

#include "geometry/latlon.hpp"
#include "geometry/rect2d.hpp"

#include "base/exception.hpp"

#include <map>
#include <string>

namespace osm
{
struct UserPreferences
{
  uint64_t m_id;
  std::string m_displayName;
  time_t m_accountCreated;
  std::string m_imageUrl;
  uint32_t m_changesets;
};

/// All methods here are synchronous and need wrappers for async usage.
/// Exceptions are used for error handling.
class ServerApi06
{
public:
  // k= and v= tags used in OSM.
  using KeyValueTags = std::map<std::string, std::string>;

  DECLARE_EXCEPTION(ServerApi06Exception, RootException);
  DECLARE_EXCEPTION(NotAuthorized, ServerApi06Exception);
  DECLARE_EXCEPTION(CantParseServerResponse, ServerApi06Exception);
  DECLARE_EXCEPTION(CreateChangeSetHasFailed, ServerApi06Exception);
  DECLARE_EXCEPTION(UpdateChangeSetHasFailed, ServerApi06Exception);
  DECLARE_EXCEPTION(CreateElementHasFailed, ServerApi06Exception);
  DECLARE_EXCEPTION(ModifiedElementHasNoIdAttribute, ServerApi06Exception);
  DECLARE_EXCEPTION(ModifyElementHasFailed, ServerApi06Exception);
  DECLARE_EXCEPTION(ErrorClosingChangeSet, ServerApi06Exception);
  DECLARE_EXCEPTION(ErrorAddingNote, ServerApi06Exception);
  DECLARE_EXCEPTION(DeletedElementHasNoIdAttribute, ServerApi06Exception);
  DECLARE_EXCEPTION(ErrorDeletingElement, ServerApi06Exception);
  DECLARE_EXCEPTION(CantGetUserPreferences, ServerApi06Exception);
  DECLARE_EXCEPTION(CantParseUserPreferences, ServerApi06Exception);

  ServerApi06(OsmOAuth const & auth);
  /// This function can be used to check if user did not confirm email validation link after registration.
  /// Throws if there is no connection.
  /// @returns true if user have registered/signed up even if his email address was not confirmed yet.
  bool TestOSMUser(std::string const & userName);
  /// Get OSM user preferences in a convenient struct.
  /// Throws in case of any error.
  UserPreferences GetUserPreferences() const;
  /// Please use at least created_by=* and comment=* tags.
  /// @returns created changeset ID.
  uint64_t CreateChangeSet(KeyValueTags const & kvTags) const;
  /// <node>, <way> or <relation> are supported.
  /// Only one element per call is supported.
  /// @returns id of created element.
  uint64_t CreateElement(editor::XMLFeature const & element) const;
  /// The same as const version but also updates id and version for passed element.
  void CreateElementAndSetAttributes(editor::XMLFeature & element) const;
  /// @param element should already have all attributes set, including "id", "version", "changeset".
  /// @returns new version of modified element.
  uint64_t ModifyElement(editor::XMLFeature const & element) const;
  /// Sets element's version.
  void ModifyElementAndSetVersion(editor::XMLFeature & element) const;
  /// Some nodes can't be deleted if they are used in ways or relations.
  /// @param element should already have all attributes set, including "id", "version", "changeset".
  /// @returns true if element was successfully deleted (or was already deleted).
  void DeleteElement(editor::XMLFeature const & element) const;
  void UpdateChangeSet(uint64_t changesetId, KeyValueTags const & kvTags) const;
  void CloseChangeSet(uint64_t changesetId) const;
  /// @returns id of a created note.
  uint64_t CreateNote(ms::LatLon const & ll, std::string const & message) const;
  void CloseNote(uint64_t const id) const;

  /// @returns OSM xml string with features in the bounding box or empty string on error.
  OsmOAuth::Response GetXmlFeaturesInRect(double minLat, double minLon, double maxLat, double maxLon) const;
  OsmOAuth::Response GetXmlFeaturesAtLatLon(double lat, double lon, double radiusInMeters = 1.0) const;
  OsmOAuth::Response GetXmlFeaturesAtLatLon(ms::LatLon const & ll, double radiusInMeters = 1.0) const;

private:
  OsmOAuth m_auth;
};

} // namespace osm
