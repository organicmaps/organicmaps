#pragma once

#include "editor/server_api.hpp"
#include "editor/xml_feature.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/exception.hpp"

#include <map>
#include <string>
#include <vector>

class FeatureType;

namespace osm
{
class ChangesetWrapper
{
  using TypeCount = std::map<std::string, size_t>;

public:
  DECLARE_EXCEPTION(ChangesetWrapperException, RootException);
  DECLARE_EXCEPTION(NetworkErrorException, ChangesetWrapperException);
  DECLARE_EXCEPTION(HttpErrorException, ChangesetWrapperException);
  DECLARE_EXCEPTION(OsmXmlParseException, ChangesetWrapperException);
  DECLARE_EXCEPTION(OsmObjectWasDeletedException, ChangesetWrapperException);
  DECLARE_EXCEPTION(CreateChangeSetFailedException, ChangesetWrapperException);
  DECLARE_EXCEPTION(ModifyNodeFailedException, ChangesetWrapperException);
  DECLARE_EXCEPTION(LinearFeaturesAreNotSupportedException, ChangesetWrapperException);
  DECLARE_EXCEPTION(EmptyFeatureException, ChangesetWrapperException);

  ChangesetWrapper(std::string const & keySecret, ServerApi06::KeyValueTags comments) noexcept;
  ~ChangesetWrapper();

  /// Throws many exceptions from above list, plus including XMLNode's parsing ones.
  /// OsmObjectWasDeletedException means that node was deleted from OSM server by someone else.
  editor::XMLFeature GetMatchingNodeFeatureFromOSM(m2::PointD const & center);
  editor::XMLFeature GetMatchingAreaFeatureFromOSM(std::vector<m2::PointD> const & geomerty);

  /// Throws exceptions from above list.
  void Create(editor::XMLFeature node);

  /// Throws exceptions from above list.
  /// Node should have correct OSM "id" attribute set.
  void Modify(editor::XMLFeature node);

  /// Throws exceptions from above list.
  void Delete(editor::XMLFeature node);

  /// Add a tag to the changeset
  void AddChangesetTag(std::string key, std::string value);

  /// Allows to see exception details in OSM changesets for easier debugging.
  void SetErrorDescription(std::string const & error);

private:
  /// Unfortunately, pugi can't return xml_documents from methods.
  /// Throws exceptions from above list.
  void LoadXmlFromOSM(ms::LatLon const & ll, pugi::xml_document & doc, double radiusInMeters = 1.0);
  void LoadXmlFromOSM(ms::LatLon const & min, ms::LatLon const & max, pugi::xml_document & doc);

  ServerApi06::KeyValueTags m_changesetComments;
  ServerApi06 m_api;
  static constexpr uint64_t kInvalidChangesetId = 0;
  uint64_t m_changesetId = kInvalidChangesetId;

  TypeCount m_modified_types;
  TypeCount m_created_types;
  TypeCount m_deleted_types;
  std::string m_error;
  static std::string TypeCountToString(TypeCount const & typeCount);
  std::string GetDescription() const;
};

}  // namespace osm
