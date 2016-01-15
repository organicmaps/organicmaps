#pragma once

#include "editor/server_api.hpp"
#include "editor/xml_feature.hpp"

#include "base/exception.hpp"

class FeatureType;

namespace osm
{

struct ClientToken;

class ChangesetWrapper
{
public:
  DECLARE_EXCEPTION(ChangesetWrapperException, RootException);
  DECLARE_EXCEPTION(NetworkErrorException, ChangesetWrapperException);
  DECLARE_EXCEPTION(HttpErrorException, ChangesetWrapperException);
  DECLARE_EXCEPTION(OsmXmlParseException, ChangesetWrapperException);
  DECLARE_EXCEPTION(OsmObjectWasDeletedException, ChangesetWrapperException);
  DECLARE_EXCEPTION(CreateChangeSetFailedException, ChangesetWrapperException);
  DECLARE_EXCEPTION(ModifyNodeFailedException, ChangesetWrapperException);
  DECLARE_EXCEPTION(LinearFeaturesAreNotSupportedException, ChangesetWrapperException);

  ChangesetWrapper(TKeySecret const & keySecret, ServerApi06::TKeyValueTags const & comments);
  ~ChangesetWrapper();

  /// Throws many exceptions, including XMLNode's parsing ones.
  /// OsmObjectWasDeletedException means that node was deleted from OSM server by someone else.
  editor::XMLFeature GetMatchingFeatureFromOSM(editor::XMLFeature const & ourPatch, FeatureType const & feature);

  /// Throws.
  void ModifyNode(editor::XMLFeature node);

private:
  ServerApi06::TKeyValueTags m_changesetComments;
  ServerApi06 m_api;
  static constexpr int const kInvalidChangesetId = 0;
  uint64_t m_changesetId = kInvalidChangesetId;
};

} // namespace osm
