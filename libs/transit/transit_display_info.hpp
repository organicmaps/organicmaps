#pragma once

#include "transit/transit_types.hpp"

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

using TransitStopsInfo = std::map<routing::transit::StopId, routing::transit::Stop>;
using TransitTransfersInfo = std::map<routing::transit::TransferId, routing::transit::Transfer>;
using TransitShapesInfo = std::map<routing::transit::ShapeId, routing::transit::Shape>;
using TransitLinesInfo = std::map<routing::transit::LineId, routing::transit::Line>;

struct TransitDisplayInfo
{
  TransitFeaturesInfo m_features;

  TransitLinesInfo m_linesSubway;
  TransitStopsInfo m_stopsSubway;
  TransitTransfersInfo m_transfersSubway;
  TransitShapesInfo m_shapesSubway;
};

using TransitDisplayInfos = std::map<MwmSet::MwmId, std::unique_ptr<TransitDisplayInfo>>;
