#pragma once

#include "transit/experimental/transit_types_experimental.hpp"
#include "transit/transit_entities.hpp"
#include "transit/transit_types.hpp"
#include "transit/transit_version.hpp"

#include "indexer/feature_decl.hpp"

#include <map>
#include <memory>
#include <string>

struct TransitFeatureInfo
{
  bool m_isGate = false;
  std::string m_gateSymbolName;
  std::string m_title;
  m2::PointD m_point;
};

using TransitFeaturesInfo = std::map<FeatureID, TransitFeatureInfo>;

// We have two completely different versions of transit section: the actual one with subway-only
// info and the experimental one with all public transport (PT) info. So we keep the old data
// with suffix Subway and the new data with suffix PT. In the future we will remove the subway
// version (after a year or so of PT usage).
using TransitStopsInfo = std::map<routing::transit::StopId, routing::transit::Stop>;
using TransitTransfersInfo = std::map<routing::transit::TransferId, routing::transit::Transfer>;
using TransitShapesInfo = std::map<routing::transit::ShapeId, routing::transit::Shape>;
using TransitLinesInfo = std::map<routing::transit::LineId, routing::transit::Line>;
using TransitNetworksInfo = std::map<routing::transit::NetworkId, routing::transit::Network>;

using TransitStopsInfoPT = std::map<::transit::TransitId, ::transit::experimental::Stop>;
using TransitTransfersInfoPT = std::map<::transit::TransitId, ::transit::experimental::Transfer>;
using TransitShapesInfoPT = std::map<::transit::TransitId, ::transit::experimental::Shape>;
using TransitLinesInfoPT = std::map<::transit::TransitId, ::transit::experimental::Line>;
using TransitLinesMetadataInfoPT = std::map<::transit::TransitId, ::transit::experimental::LineMetadata>;
using TransitRoutesInfoPT = std::map<::transit::TransitId, ::transit::experimental::Route>;
using TransitNetworksInfoPT = std::map<::transit::TransitId, ::transit::experimental::Network>;
using TransitEdgesInfoPT = std::unordered_map<::transit::EdgeId, ::transit::EdgeData, ::transit::EdgeIdHasher>;
struct TransitDisplayInfo
{
  ::transit::TransitVersion m_transitVersion;

  TransitFeaturesInfo m_features;

  TransitNetworksInfo m_networksSubway;
  TransitLinesInfo m_linesSubway;
  TransitStopsInfo m_stopsSubway;
  TransitTransfersInfo m_transfersSubway;
  TransitShapesInfo m_shapesSubway;

  TransitNetworksInfoPT m_networksPT;
  TransitLinesInfoPT m_linesPT;
  TransitLinesMetadataInfoPT m_linesMetadataPT;
  TransitRoutesInfoPT m_routesPT;
  TransitStopsInfoPT m_stopsPT;
  TransitTransfersInfoPT m_transfersPT;
  TransitShapesInfoPT m_shapesPT;
  TransitEdgesInfoPT m_edgesPT;
};

using TransitDisplayInfos = std::map<MwmSet::MwmId, std::unique_ptr<TransitDisplayInfo>>;
