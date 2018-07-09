#pragma once

#include "transit/transit_types.hpp"

#include "indexer/feature_decl.hpp"

#include <map>
#include <string>

struct TransitFeatureInfo
{
  bool m_isGate = false;
  std::string m_gateSymbolName;
  std::string m_title;
  m2::PointD m_point;
};

using TransitFeaturesInfo = std::map<FeatureID, TransitFeatureInfo>;

using TransitStopsInfo = std::map<routing::transit::StopId, routing::transit::Stop>;
using TransitTransfersInfo = std::map<routing::transit::TransferId, routing::transit::Transfer>;
using TransitShapesInfo = std::map<routing::transit::ShapeId, routing::transit::Shape>;
using TransitLinesInfo = std::map<routing::transit::LineId, routing::transit::Line>;
using TransitNetworksInfo = std::map<routing::transit::NetworkId, routing::transit::Network>;

struct TransitDisplayInfo
{
  TransitNetworksInfo m_networks;
  TransitLinesInfo m_lines;
  TransitStopsInfo m_stops;
  TransitTransfersInfo m_transfers;
  TransitShapesInfo m_shapes;
  TransitFeaturesInfo m_features;
};

using TransitDisplayInfos = std::map<MwmSet::MwmId, unique_ptr<TransitDisplayInfo>>;
