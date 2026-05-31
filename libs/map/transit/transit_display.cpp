#include "transit_display.hpp"

#include "drape_frontend/route_renderer.hpp"
#include "drape_frontend/visual_params.hpp"

#include "routing/routing_session.hpp"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <vector>

std::map<TransitType, std::string> const kTransitSymbols = {{TransitType::Subway, "transit_subway"},
                                                            {TransitType::LightRail, "transit_light_rail"},
                                                            {TransitType::Monorail, "transit_monorail"},
                                                            {TransitType::Train, "transit_train"},
                                                            {TransitType::Tram, "transit_tram"},
                                                            {TransitType::Bus, "transit_bus"},
                                                            {TransitType::Ferry, "transit_ferry"},
                                                            {TransitType::CableTram, "transit_cable_tram"},
                                                            {TransitType::AerialLift, "transit_aerial_lift"},
                                                            {TransitType::Funicular, "transit_funicular"},
                                                            // Trolleybuses reuse the bus icon (no dedicated
                                                            // trolleybus symbol; cities don't distinguish them).
                                                            {TransitType::Trolleybus, "transit_bus"},
                                                            {TransitType::AirService, "transit_air_service"},
                                                            {TransitType::WaterService, "transit_water_service"}};

std::string DebugPrint(TransitType type)
{
  switch (type)
  {
  case TransitType::IntermediatePoint: return "IntermediatePoint";
  case TransitType::Pedestrian: return "Pedestrian";
  case TransitType::Subway: return "Subway";
  case TransitType::Train: return "Train";
  case TransitType::LightRail: return "LightRail";
  case TransitType::Monorail: return "Monorail";
  case TransitType::Tram: return "Tram";
  case TransitType::Bus: return "Bus";
  case TransitType::Ferry: return "Ferry";
  case TransitType::CableTram: return "CableTram";
  case TransitType::AerialLift: return "AerialLift";
  case TransitType::Funicular: return "Funicular";
  case TransitType::Trolleybus: return "Trolleybus";
  case TransitType::AirService: return "AirService";
  case TransitType::WaterService: return "WaterService";
  }

  UNREACHABLE();
}

std::string DebugPrint(TransitStepInfo const & info)
{
  std::ostringstream os;
  os << "TransitStep [" << DebugPrint(info.m_type);
  if (!info.m_number.empty())
    os << " #" << info.m_number;
  os << ", " << info.m_distanceInMeters << " m, " << info.m_timeInSec << " s";
  if (info.m_type == TransitType::IntermediatePoint)
    os << ", intermediateIdx=" << info.m_intermediateIndex;
  os << "]";
  return os.str();
}

namespace
{
float const kStopMarkerScale = 2.2f;
float const kTransferMarkerScale = 4.0f;
float const kGateBgScale = 1.2f;

int const kSmallIconZoom = 1;
int const kMediumIconZoom = 10;

int const kMinStopTitleZoom = 13;

int const kTransferTitleOffset = 2;
int const kStopTitleOffset = 0;
int const kGateTitleOffset = 0;

std::string const kZeroIcon = "zero-icon";

TransitType GetTransitType(std::string const & type)
{
  if (type == "subway")
    return TransitType::Subway;
  if (type == "light_rail")
    return TransitType::LightRail;
  if (type == "monorail")
    return TransitType::Monorail;
  if (type == "train" || type == "rail")
    return TransitType::Train;

  if (type == "tram")
    return TransitType::Tram;
  if (type == "bus")
    return TransitType::Bus;
  if (type == "ferry")
    return TransitType::Ferry;
  if (type == "cabletram")
    return TransitType::CableTram;
  if (type == "aerial_lift")
    return TransitType::AerialLift;
  if (type == "funicular")
    return TransitType::Funicular;
  if (type == "trolleybus")
    return TransitType::Trolleybus;
  if (type == "air_service")
    return TransitType::AirService;
  if (type == "water_service")
    return TransitType::WaterService;

  UNREACHABLE();
}

namespace helpers
{
using Line = routing::transit::Line;
using StopId = routing::transit::StopId;

// True if |line| visits |boardId| and then |alightId| later in the same stop range. Riders board and
// alight at the same stops; intermediate stops may differ between parallel lines, which is fine.
bool LineVisitsInOrder(Line const & line, StopId boardId, StopId alightId)
{
  for (auto const & range : line.GetStopIds())
  {
    auto const boardIt = std::find(range.cbegin(), range.cend(), boardId);
    if (boardIt != range.cend() && std::find(boardIt + 1, range.cend(), alightId) != range.cend())
      return true;
  }
  return false;
}

// Numbers (e.g. "1, 5, 12") of all |type| lines a rider can take for the whole leg, i.e. lines that
// go from the boarding stop |boardId| to the alighting stop |alightId| (in order). Computed per leg
// rather than per edge so parallel lines with different intermediate stops still merge into one step.
// Numbers are deduplicated and sorted by their leading integer.
std::string GetSharedLineNumbers(TransitDisplayInfo const & info, StopId boardId, StopId alightId, TransitType type)
{
  std::vector<std::string> numbers;
  for (auto const & [_, line] : info.m_linesSubway)
  {
    if (GetTransitType(line.GetType()) != type || !LineVisitsInOrder(line, boardId, alightId))
      continue;
    auto const & number = line.GetNumber();
    if (!number.empty() && std::find(numbers.cbegin(), numbers.cend(), number) == numbers.cend())
      numbers.push_back(number);
  }

  std::sort(numbers.begin(), numbers.end(), [](std::string const & lhs, std::string const & rhs)
  {
    char * end = nullptr;
    long const l = std::strtol(lhs.c_str(), &end, 10);
    long const r = std::strtol(rhs.c_str(), &end, 10);
    if (l != r)
      return l < r;
    return lhs < rhs;
  });

  std::string result;
  for (auto const & number : numbers)
  {
    if (!result.empty())
      result += ", ";
    result += number;
  }
  return result;
}

// For every subway Edge segment in |segments|, the boarding and alighting stops of its leg (a maximal
// run of consecutive edges on the same line). Non-edge segments get {kInvalidStopId, kInvalidStopId}.
std::vector<std::pair<StopId, StopId>> ComputeSubwayLegStops(std::vector<routing::RouteSegment> const & segments)
{
  std::vector<std::pair<StopId, StopId>> legs(segments.size(),
                                              {routing::transit::kInvalidStopId, routing::transit::kInvalidStopId});

  auto const isSubwayEdge = [](routing::RouteSegment const & s)
  {
    return s.HasTransitInfo() && s.GetTransitInfo().GetVersion() == ::transit::TransitVersion::OnlySubway &&
           s.GetTransitInfo().GetType() == routing::TransitInfo::Type::Edge;
  };

  for (size_t i = 0; i < segments.size();)
  {
    if (!isSubwayEdge(segments[i]))
    {
      ++i;
      continue;
    }

    auto const lineId = segments[i].GetTransitInfo().GetEdgeSubway().m_lineId;
    StopId const board = segments[i].GetTransitInfo().GetEdgeSubway().m_stop1Id;
    size_t j = i;
    StopId alight = board;
    while (j < segments.size() && isSubwayEdge(segments[j]) &&
           segments[j].GetTransitInfo().GetEdgeSubway().m_lineId == lineId)
    {
      alight = segments[j].GetTransitInfo().GetEdgeSubway().m_stop2Id;
      ++j;
    }
    for (size_t k = i; k < j; ++k)
      legs[k] = {board, alight};
    i = j;
  }
  return legs;
}
}  // namespace helpers

uint32_t ColorToARGB(df::ColorConstant const & colorConstant)
{
  auto const color = df::GetColorConstant(colorConstant);
  return color.GetAlpha() << 24 | color.GetRed() << 16 | color.GetGreen() << 8 | color.GetBlue();
}

std::vector<m2::PointF> GetTransitMarkerSizes(float markerScale, float maxRouteWidth)
{
  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());
  std::vector<m2::PointF> markerSizes;
  markerSizes.reserve(df::kRouteHalfWidthInPixelTransit.size());
  for (auto const halfWidth : df::kRouteHalfWidthInPixelTransit)
  {
    float const d = 2.0f * std::min(halfWidth * vs, maxRouteWidth * 0.5f) * markerScale;
    markerSizes.push_back(m2::PointF(d, d));
  }
  return markerSizes;
}
}  // namespace

TransitStepInfo::TransitStepInfo(TransitType type, double distance, int time, std::string const & number,
                                 uint32_t color, int intermediateIndex, std::string const & startStopName,
                                 std::string const & endStopName)
  : m_type(type)
  , m_distanceInMeters(distance)
  , m_timeInSec(time)
  , m_number(number)
  , m_colorARGB(color)
  , m_intermediateIndex(intermediateIndex)
  , m_startStopName(startStopName)
  , m_endStopName(endStopName)
  , m_stopCount(type != TransitType::Pedestrian && type != TransitType::IntermediatePoint ? 1 : 0)
{}

bool TransitStepInfo::IsEqualType(TransitStepInfo const & ts) const
{
  if (m_type != ts.m_type)
    return false;

  if (m_type != TransitType::Pedestrian && m_type != TransitType::IntermediatePoint)
    return m_number == ts.m_number && m_colorARGB == ts.m_colorARGB;
  return true;
}

void TransitRouteInfo::AddStep(TransitStepInfo const & step)
{
  if (!m_steps.empty() && m_steps.back().IsEqualType(step))
  {
    auto & back = m_steps.back();
    back.m_distanceInMeters += step.m_distanceInMeters;
    back.m_timeInSec += step.m_timeInSec;
    if (!step.m_endStopName.empty())
    {
      if (!back.m_endStopName.empty())
        back.m_intermediateStopNames.push_back(back.m_endStopName);
      back.m_endStopName = step.m_endStopName;
    }
    back.m_stopCount += step.m_stopCount;
  }
  else
  {
    m_steps.push_back(step);
  }

  if (step.m_type == TransitType::Pedestrian)
  {
    m_totalPedestrianDistInMeters += step.m_distanceInMeters;
    m_totalPedestrianTimeInSec += step.m_timeInSec;
  }
}

void TransitRouteInfo::UpdateDistanceStrings()
{
  if (m_steps.empty())
    return;
  for (auto & step : m_steps)
    routing::FormatDistance(step.m_distanceInMeters, step.m_distanceStr, step.m_distanceUnitsSuffix);
  routing::FormatDistance(m_totalDistInMeters, m_totalDistanceStr, m_totalDistanceUnitsSuffix);
  routing::FormatDistance(m_totalPedestrianDistInMeters, m_totalPedestrianDistanceStr, m_totalPedestrianUnitsSuffix);
}

// Appends |destPoint| to the subroute polyline as a single segment styled with |style|.
void AddSegment(m2::PointD const & destPoint, df::SubrouteStyle style, df::Subroute & subroute)
{
  ASSERT_GREATER(subroute.m_polyline.GetSize(), 0, ());
  style.m_startIndex = subroute.m_polyline.GetSize() - 1;
  subroute.m_polyline.Add(destPoint);
  style.m_endIndex = subroute.m_polyline.GetSize() - 1;
  subroute.AddStyle(style);
}

void AddTransitGateSegment(m2::PointD const & destPoint, std::string const & color, df::Subroute & subroute)
{
  AddSegment(destPoint, df::SubrouteStyle(color, df::RoutePattern(4.0, 2.0)), subroute);
}

void AddTransitPedestrianSegment(m2::PointD const & destPoint, df::Subroute & subroute)
{
  AddSegment(destPoint, df::SubrouteStyle(df::kRoutePedestrian, df::RoutePattern(4.0, 2.0)), subroute);
}

// Bus/tram edges are stored without shapes, the route is drawn as a straight line to the next stop.
void AddTransitStraightSegment(m2::PointD const & destPoint, std::string const & color, df::Subroute & subroute)
{
  AddSegment(destPoint, df::SubrouteStyle(color), subroute);
}

void AddTransitShapes(std::vector<routing::transit::ShapeId> const & shapeIds, TransitShapesInfo const & shapes,
                      std::string const & color, bool isInverted, df::Subroute & subroute)
{
  ASSERT_GREATER(subroute.m_polyline.GetSize(), 0, ());
  df::SubrouteStyle style(color);
  style.m_startIndex = subroute.m_polyline.GetSize() - 1;

  for (auto it = shapeIds.crbegin(); it != shapeIds.crend(); ++it)
  {
    auto const & shapePolyline = shapes.at(*it).GetPolyline();
    if (isInverted)
      subroute.m_polyline.Append(shapePolyline.crbegin(), shapePolyline.crend());
    else
      subroute.m_polyline.Append(shapePolyline.cbegin(), shapePolyline.cend());
  }

  style.m_endIndex = subroute.m_polyline.GetSize() - 1;
  subroute.AddStyle(style);
}

void AddTransitShapes(::transit::ShapeLink shapeLink, TransitShapesInfoPT const & shapesInfo, std::string const & color,
                      df::Subroute & subroute)
{
  ASSERT_GREATER(subroute.m_polyline.GetSize(), 0, ());
  df::SubrouteStyle style(color);
  style.m_startIndex = subroute.m_polyline.GetSize() - 1;

  bool const isInverted = shapeLink.m_startIndex > shapeLink.m_endIndex;

  auto const it = shapesInfo.find(shapeLink.m_shapeId);
  CHECK(it != shapesInfo.end(), (shapeLink.m_shapeId));

  size_t const startIdx = isInverted ? shapeLink.m_endIndex : shapeLink.m_startIndex;
  size_t const endIdx = isInverted ? shapeLink.m_startIndex : shapeLink.m_endIndex;

  auto const & edgePolyline = std::vector<m2::PointD>(it->second.GetPolyline().begin() + startIdx,
                                                      it->second.GetPolyline().begin() + endIdx + 1);

  if (isInverted)
    subroute.m_polyline.Append(edgePolyline.crbegin(), edgePolyline.crend());
  else
    subroute.m_polyline.Append(edgePolyline.cbegin(), edgePolyline.cend());

  style.m_endIndex = subroute.m_polyline.GetSize() - 1;
  subroute.AddStyle(style);
}

TransitRouteDisplay::TransitRouteDisplay(TransitReadManager & transitReadManager, GetMwmIdFn const & getMwmIdFn,
                                         GetStringsBundleFn const & getStringsBundleFn, BookmarkManager * bmManager,
                                         std::map<std::string, m2::PointF> const & transitSymbolSizes)
  : m_transitReadManager(transitReadManager)
  , m_getMwmIdFn(getMwmIdFn)
  , m_getStringsBundleFn(getStringsBundleFn)
  , m_bmManager(bmManager)
  , m_symbolSizes(transitSymbolSizes)
{
  float maxSymbolSize = -1.0f;
  for (auto const & symbolSize : m_symbolSizes)
  {
    if (maxSymbolSize < symbolSize.second.x)
      maxSymbolSize = symbolSize.second.x;
    if (maxSymbolSize < symbolSize.second.y)
      maxSymbolSize = symbolSize.second.y;
  }
  m_maxSubrouteWidth = maxSymbolSize * kGateBgScale / kStopMarkerScale;
}

TransitRouteInfo const & TransitRouteDisplay::GetRouteInfo()
{
  m_routeInfo.UpdateDistanceStrings();
  LOG(LDEBUG, ("Transit route:", m_routeInfo.m_steps.size(), "steps,", m_routeInfo.m_totalDistInMeters, "m,",
               m_routeInfo.m_totalTimeInSec, "s, steps:", m_routeInfo.m_steps));
  return m_routeInfo;
}

namespace
{
template <class Stop>
std::string GetStopName(SubrouteSegmentParams const & ssp, Stop const & stop)
{
  if (stop.GetFeatureId() == kInvalidFeatureId)
    return {};
  auto const fid = FeatureID(ssp.m_mwmId, stop.GetFeatureId());
  auto const it = ssp.m_displayInfo.m_features.find(fid);
  if (it == ssp.m_displayInfo.m_features.end())
    return {};
  return it->second.m_title;
}
}  // namespace

void TransitRouteDisplay::AddEdgeSubwayForSubroute(routing::RouteSegment const & segment, df::Subroute & subroute,
                                                   SubrouteParams & sp, SubrouteSegmentParams & ssp, StopId legBoardId,
                                                   StopId legAlightId)
{
  ASSERT_EQUAL(ssp.m_displayInfo.m_transitVersion, ::transit::TransitVersion::OnlySubway, ());

  auto const & edge = ssp.m_transitInfo.GetEdgeSubway();

  auto const currentLineId = edge.m_lineId;

  auto const & line = ssp.m_displayInfo.m_linesSubway.at(currentLineId);
  auto const currentColor = df::GetTransitColorName(line.GetColor());
  sp.m_transitType = GetTransitType(line.GetType());

  // List all parallel lines a rider can take for this whole leg (same boarding and alighting stops),
  // not just the one A* picked, so they can board whichever bus comes first. The leg's board/alight
  // stops are used (not this edge's), so lines with different intermediate stops still merge into a
  // single step.
  std::string number = helpers::GetSharedLineNumbers(ssp.m_displayInfo, legBoardId, legAlightId, sp.m_transitType);
  if (number.empty())
    number = line.GetNumber();

  auto const & stop1 = ssp.m_displayInfo.m_stopsSubway.at(edge.m_stop1Id);
  auto const & stop2 = ssp.m_displayInfo.m_stopsSubway.at(edge.m_stop2Id);

  m_routeInfo.AddStep(TransitStepInfo(sp.m_transitType, ssp.m_distance, ssp.m_time, number, ColorToARGB(currentColor),
                                      0 /* intermediateIndex */, GetStopName(ssp, stop1), GetStopName(ssp, stop2)));
  bool const isTransfer1 = stop1.GetTransferId() != routing::transit::kInvalidTransferId;
  bool const isTransfer2 = stop2.GetTransferId() != routing::transit::kInvalidTransferId;

  sp.m_marker.m_distance = sp.m_prevDistance;
  sp.m_marker.m_scale = kStopMarkerScale;
  sp.m_marker.m_innerColor = currentColor;
  if (isTransfer1)
  {
    auto const & transferData = ssp.m_displayInfo.m_transfersSubway.at(stop1.GetTransferId());
    sp.m_marker.m_position = transferData.GetPoint();
  }
  else
  {
    sp.m_marker.m_position = stop1.GetPoint();
  }
  sp.m_transitMarkInfo.m_point = sp.m_marker.m_position;

  if (sp.m_pendingEntrance)
  {
    sp.m_transitMarkInfo.m_type = TransitMarkInfo::Type::KeyStop;
    sp.m_transitMarkInfo.m_symbolName = kTransitSymbols.at(sp.m_transitType);
    sp.m_transitMarkInfo.m_color = currentColor;
    AddTransitGateSegment(sp.m_marker.m_position, currentColor, subroute);
    sp.m_pendingEntrance = false;
  }

  auto const id1 = isTransfer1 ? stop1.GetTransferId() : stop1.GetId();
  auto const id2 = isTransfer2 ? stop2.GetTransferId() : stop2.GetId();

  if (id1 != id2)
  {
    if (edge.m_shapeIds.empty())
    {
      auto const & destPoint =
          isTransfer2 ? ssp.m_displayInfo.m_transfersSubway.at(stop2.GetTransferId()).GetPoint() : stop2.GetPoint();
      AddTransitStraightSegment(destPoint, currentColor, subroute);
    }
    else
    {
      bool const isInverted = id1 > id2;
      AddTransitShapes(edge.m_shapeIds, ssp.m_displayInfo.m_shapesSubway, currentColor, isInverted, subroute);
    }
  }

  CHECK_GREATER(subroute.m_polyline.GetSize(), 1, ());
  auto const & p1 = *(subroute.m_polyline.End() - 2);
  auto const & p2 = *(subroute.m_polyline.End() - 1);
  m2::PointD currentDir = (p2 - p1).Normalize();

  if (sp.m_lastLineId != currentLineId)
  {
    if (sp.m_lastLineId != routing::transit::kInvalidLineId)
    {
      sp.m_marker.m_scale = kTransferMarkerScale;
      sp.m_transitMarkInfo.m_type = TransitMarkInfo::Type::Transfer;
    }
    sp.m_marker.m_colors.push_back(currentColor);

    if (stop1.GetFeatureId() != kInvalidFeatureId)
    {
      auto const fid = FeatureID(ssp.m_mwmId, stop1.GetFeatureId());
      sp.m_transitMarkInfo.m_featureId = fid;
      sp.m_transitMarkInfo.m_titles.emplace_back(ssp.m_displayInfo.m_features.at(fid).m_title,
                                                 df::GetTransitTextColorName(line.GetColor()));
    }
  }
  sp.m_lastColor = currentColor;
  sp.m_lastLineId = currentLineId;

  if (sp.m_marker.m_colors.size() > 1)
  {
    sp.m_marker.m_innerColor = df::kTransitStopInnerMarkerColor;
    sp.m_marker.m_up = (currentDir - sp.m_lastDir).Normalize();
    if (m2::CrossProduct(sp.m_marker.m_up, -sp.m_lastDir) < 0)
      sp.m_marker.m_up = -sp.m_marker.m_up;
  }

  subroute.m_markers.push_back(sp.m_marker);
  sp.m_marker = df::SubrouteMarker();

  m_transitMarks.push_back(sp.m_transitMarkInfo);
  sp.m_transitMarkInfo = TransitMarkInfo();

  sp.m_lastDir = currentDir;

  sp.m_marker.m_distance = segment.GetDistFromBeginningMeters();
  sp.m_marker.m_scale = kStopMarkerScale;
  sp.m_marker.m_innerColor = currentColor;
  sp.m_marker.m_colors.push_back(currentColor);

  if (isTransfer2)
  {
    auto const & transferData = ssp.m_displayInfo.m_transfersSubway.at(stop2.GetTransferId());
    sp.m_marker.m_position = transferData.GetPoint();
  }
  else
  {
    sp.m_marker.m_position = stop2.GetPoint();
  }

  sp.m_transitMarkInfo.m_point = sp.m_marker.m_position;
  if (stop2.GetFeatureId() != kInvalidFeatureId)
  {
    auto const fid = FeatureID(ssp.m_mwmId, stop2.GetFeatureId());
    sp.m_transitMarkInfo.m_featureId = fid;
    sp.m_transitMarkInfo.m_titles.emplace_back(ssp.m_displayInfo.m_features.at(fid).m_title,
                                               df::GetTransitTextColorName(line.GetColor()));
  }
}

void TransitRouteDisplay::AddEdgePTForSubroute(routing::RouteSegment const & segment, df::Subroute & subroute,
                                               SubrouteParams & sp, SubrouteSegmentParams & ssp)
{
  ASSERT_EQUAL(ssp.m_displayInfo.m_transitVersion, ::transit::TransitVersion::AllPublicTransport, ());

  auto const & edge = ssp.m_transitInfo.GetEdgePT();

  auto const currentLineId = edge.m_lineId;
  auto const & line = ssp.m_displayInfo.m_linesPT.at(currentLineId);

  auto const it = ssp.m_displayInfo.m_routesPT.find(line.GetRouteId());
  CHECK(it != ssp.m_displayInfo.m_routesPT.end(), (line.GetRouteId()));
  auto const & route = it->second;

  auto const currentColor = df::GetTransitColorName(route.GetColor());
  sp.m_transitType = GetTransitType(route.GetType());

  auto const & stop1 = ssp.m_displayInfo.m_stopsPT.at(edge.m_stop1Id);
  auto const & stop2 = ssp.m_displayInfo.m_stopsPT.at(edge.m_stop2Id);

  m_routeInfo.AddStep(TransitStepInfo(sp.m_transitType, ssp.m_distance, ssp.m_time, route.GetTitle(),
                                      ColorToARGB(currentColor), 0 /* intermediateIndex */, GetStopName(ssp, stop1),
                                      GetStopName(ssp, stop2)));

  bool const isTransfer1 = !stop1.GetTransferIds().empty();
  bool const isTransfer2 = !stop2.GetTransferIds().empty();

  sp.m_marker.m_distance = sp.m_prevDistance;
  sp.m_marker.m_scale = kStopMarkerScale;
  sp.m_marker.m_innerColor = currentColor;

  if (isTransfer1)
  {
    auto itTransfer = ssp.m_displayInfo.m_transfersPT.find(stop1.GetTransferIds().front());
    ASSERT(itTransfer != ssp.m_displayInfo.m_transfersPT.end(), ());
    sp.m_marker.m_position = itTransfer->second.GetPoint();
  }
  else
  {
    sp.m_marker.m_position = stop1.GetPoint();
  }

  sp.m_transitMarkInfo.m_point = sp.m_marker.m_position;

  if (sp.m_pendingEntrance)
  {
    sp.m_transitMarkInfo.m_type = TransitMarkInfo::Type::KeyStop;
    sp.m_transitMarkInfo.m_symbolName = kTransitSymbols.at(sp.m_transitType);
    sp.m_transitMarkInfo.m_color = currentColor;
    AddTransitGateSegment(sp.m_marker.m_position, currentColor, subroute);
    sp.m_pendingEntrance = false;
  }

  auto const id1 = isTransfer1 ? stop1.GetTransferIds().front() : stop1.GetId();
  auto const id2 = isTransfer2 ? stop2.GetTransferIds().front() : stop2.GetId();

  if (id1 != id2)
    AddTransitShapes(edge.m_shapeLink, ssp.m_displayInfo.m_shapesPT, currentColor, subroute);

  CHECK_GREATER(subroute.m_polyline.GetSize(), 1, ());
  auto const & p1 = *(subroute.m_polyline.End() - 2);
  auto const & p2 = *(subroute.m_polyline.End() - 1);
  m2::PointD currentDir = (p2 - p1).Normalize();

  if (sp.m_lastLineId != currentLineId)
  {
    if (sp.m_lastLineId != routing::transit::kInvalidLineId)
    {
      sp.m_marker.m_scale = kTransferMarkerScale;
      sp.m_transitMarkInfo.m_type = TransitMarkInfo::Type::Transfer;
    }
    sp.m_marker.m_colors.push_back(currentColor);

    if (stop1.GetFeatureId() != kInvalidFeatureId)
    {
      auto const fid = FeatureID(ssp.m_mwmId, stop1.GetFeatureId());
      sp.m_transitMarkInfo.m_featureId = fid;
      sp.m_transitMarkInfo.m_titles.emplace_back(ssp.m_displayInfo.m_features.at(fid).m_title,
                                                 df::GetTransitTextColorName(route.GetColor()));
    }
  }
  sp.m_lastColor = currentColor;
  sp.m_lastLineId = currentLineId;

  if (sp.m_marker.m_colors.size() > 1)
  {
    sp.m_marker.m_innerColor = df::kTransitStopInnerMarkerColor;
    sp.m_marker.m_up = (currentDir - sp.m_lastDir).Normalize();
    if (m2::CrossProduct(sp.m_marker.m_up, -sp.m_lastDir) < 0)
      sp.m_marker.m_up = -sp.m_marker.m_up;
  }

  subroute.m_markers.push_back(sp.m_marker);
  sp.m_marker = df::SubrouteMarker();

  m_transitMarks.push_back(sp.m_transitMarkInfo);
  sp.m_transitMarkInfo = TransitMarkInfo();

  sp.m_lastDir = currentDir;

  sp.m_marker.m_distance = segment.GetDistFromBeginningMeters();
  sp.m_marker.m_scale = kStopMarkerScale;
  sp.m_marker.m_innerColor = currentColor;
  sp.m_marker.m_colors.push_back(currentColor);

  if (isTransfer2)
  {
    auto itTransfer = ssp.m_displayInfo.m_transfersPT.find(stop2.GetTransferIds().front());
    ASSERT(itTransfer != ssp.m_displayInfo.m_transfersPT.end(), ());
    sp.m_marker.m_position = itTransfer->second.GetPoint();
  }
  else
  {
    sp.m_marker.m_position = stop2.GetPoint();
  }

  sp.m_transitMarkInfo.m_point = sp.m_marker.m_position;
  if (stop2.GetFeatureId() != kInvalidFeatureId)
  {
    auto const fid = FeatureID(ssp.m_mwmId, stop2.GetFeatureId());
    sp.m_transitMarkInfo.m_featureId = fid;
    sp.m_transitMarkInfo.m_titles.emplace_back(ssp.m_displayInfo.m_features.at(fid).m_title,
                                               df::GetTransitTextColorName(route.GetColor()));
  }
}

void TransitRouteDisplay::AddGateSubwayForSubroute(routing::RouteSegment const & segment, df::Subroute & subroute,
                                                   SubrouteParams & sp, SubrouteSegmentParams & ssp)
{
  auto const & gate = ssp.m_transitInfo.GetGateSubway();
  if (sp.m_lastLineId != routing::transit::kInvalidLineId)
  {
    m_routeInfo.AddStep(TransitStepInfo(TransitType::Pedestrian, ssp.m_distance, ssp.m_time));

    AddTransitGateSegment(segment.GetJunction().GetPoint(), sp.m_lastColor, subroute);

    subroute.m_markers.push_back(sp.m_marker);
    sp.m_marker = df::SubrouteMarker();

    sp.m_transitMarkInfo.m_type = TransitMarkInfo::Type::KeyStop;
    sp.m_transitMarkInfo.m_symbolName = kTransitSymbols.at(sp.m_transitType);
    sp.m_transitMarkInfo.m_color = sp.m_lastColor;
    m_transitMarks.push_back(sp.m_transitMarkInfo);
    sp.m_transitMarkInfo = TransitMarkInfo();
  }
  else
  {
    sp.m_pendingEntrance = true;
  }

  auto gateMarkInfo = TransitMarkInfo();
  gateMarkInfo.m_point = sp.m_pendingEntrance ? subroute.m_polyline.Back() : segment.GetJunction().GetPoint();
  gateMarkInfo.m_type = TransitMarkInfo::Type::Gate;
  gateMarkInfo.m_symbolName = kZeroIcon;
  if (gate.m_featureId != kInvalidFeatureId)
  {
    auto const fid = FeatureID(ssp.m_mwmId, gate.m_featureId);
    auto const & featureInfo = ssp.m_displayInfo.m_features.at(fid);
    auto symbolName = featureInfo.m_gateSymbolName;
    if (symbolName.ends_with("-s") || symbolName.ends_with("-m") || symbolName.ends_with("-l"))
      symbolName = symbolName.substr(0, symbolName.rfind('-'));

    gateMarkInfo.m_featureId = fid;
    if (!symbolName.empty())
      gateMarkInfo.m_symbolName = symbolName;
    auto const title = m_getStringsBundleFn().GetString(sp.m_pendingEntrance ? "core_entrance" : "core_exit");
    gateMarkInfo.m_titles.emplace_back(title, df::GetTransitTextColorName("default"));
  }

  m_transitMarks.push_back(gateMarkInfo);
}

void TransitRouteDisplay::AddGatePTForSubroute(routing::RouteSegment const & segment, df::Subroute & subroute,
                                               SubrouteParams & sp, SubrouteSegmentParams & ssp)
{
  auto const & gate = ssp.m_transitInfo.GetGatePT();
  if (sp.m_lastLineId != ::transit::kInvalidTransitId)
  {
    m_routeInfo.AddStep(TransitStepInfo(TransitType::Pedestrian, ssp.m_distance, ssp.m_time));

    AddTransitGateSegment(segment.GetJunction().GetPoint(), sp.m_lastColor, subroute);

    subroute.m_markers.push_back(sp.m_marker);
    sp.m_marker = df::SubrouteMarker();

    sp.m_transitMarkInfo.m_type = TransitMarkInfo::Type::KeyStop;
    sp.m_transitMarkInfo.m_symbolName = kTransitSymbols.at(sp.m_transitType);
    sp.m_transitMarkInfo.m_color = sp.m_lastColor;
    m_transitMarks.push_back(sp.m_transitMarkInfo);
    sp.m_transitMarkInfo = TransitMarkInfo();
  }
  else
  {
    sp.m_pendingEntrance = true;
  }

  auto gateMarkInfo = TransitMarkInfo();
  gateMarkInfo.m_point = sp.m_pendingEntrance ? subroute.m_polyline.Back() : segment.GetJunction().GetPoint();
  gateMarkInfo.m_type = TransitMarkInfo::Type::Gate;
  gateMarkInfo.m_symbolName = kZeroIcon;
  if (gate.m_featureId != kInvalidFeatureId)
  {
    auto const fid = FeatureID(ssp.m_mwmId, gate.m_featureId);
    auto const & featureInfo = ssp.m_displayInfo.m_features.at(fid);
    auto symbolName = featureInfo.m_gateSymbolName;
    if (symbolName.ends_with("-s") || symbolName.ends_with("-m") || symbolName.ends_with("-l"))
      symbolName = symbolName.substr(0, symbolName.rfind('-'));

    gateMarkInfo.m_featureId = fid;
    if (!symbolName.empty())
      gateMarkInfo.m_symbolName = symbolName;
    auto const title = m_getStringsBundleFn().GetString(sp.m_pendingEntrance ? "core_entrance" : "core_exit");
    gateMarkInfo.m_titles.emplace_back(title, df::GetTransitTextColorName("default"));
  }

  m_transitMarks.push_back(gateMarkInfo);
}

bool TransitRouteDisplay::ProcessSubroute(std::vector<routing::RouteSegment> const & segments, df::Subroute & subroute)
{
  if (m_subrouteIndex > 0)
  {
    TransitStepInfo step;
    step.m_type = TransitType::IntermediatePoint;
    step.m_intermediateIndex = m_subrouteIndex - 1;
    m_routeInfo.AddStep(step);
  }

  ++m_subrouteIndex;

  TransitDisplayInfos transitDisplayInfos;
  CollectTransitDisplayInfo(segments, transitDisplayInfos);

  // Read transit display info.
  if (!m_transitReadManager.GetTransitDisplayInfo(transitDisplayInfos))
    return false;

  subroute.m_maxPixelWidth = m_maxSubrouteWidth;
  subroute.m_styleType = df::SubrouteStyleType::Multiple;
  subroute.m_style.clear();
  subroute.m_style.reserve(segments.size());

  SubrouteParams sp;
  sp.m_prevDistance = m_routeInfo.m_totalDistInMeters;
  sp.m_prevTime = m_routeInfo.m_totalTimeInSec;

  // Boarding/alighting stops per subway leg, so parallel lines are enumerated for the whole leg.
  auto const legStops = helpers::ComputeSubwayLegStops(segments);

  for (size_t i = 0; i < segments.size(); ++i)
  {
    auto const & s = segments[i];
    auto const time = static_cast<int>(ceil(s.GetTimeFromBeginningSec() - sp.m_prevTime));
    auto const distance = s.GetDistFromBeginningMeters() - sp.m_prevDistance;
    sp.m_prevDistance = s.GetDistFromBeginningMeters();
    sp.m_prevTime = s.GetTimeFromBeginningSec();

    if (!s.HasTransitInfo())
    {
      m_routeInfo.AddStep(TransitStepInfo(TransitType::Pedestrian, distance, time));

      AddTransitPedestrianSegment(s.GetJunction().GetPoint(), subroute);
      sp.m_lastColor = "";
      sp.m_lastLineId = routing::transit::kInvalidLineId;
      sp.m_transitType = TransitType::Pedestrian;
      continue;
    }

    SubrouteSegmentParams ssp(s.GetTransitInfo());

    ssp.m_time = time;
    ssp.m_distance = distance;
    ssp.m_mwmId = m_getMwmIdFn(s.GetSegment().GetMwmId());
    ssp.m_displayInfo = *transitDisplayInfos.at(ssp.m_mwmId).get();

    if (ssp.m_transitInfo.GetVersion() == ::transit::TransitVersion::OnlySubway)
    {
      if (ssp.m_transitInfo.GetType() == routing::TransitInfo::Type::Edge)
        AddEdgeSubwayForSubroute(s, subroute, sp, ssp, legStops[i].first, legStops[i].second);
      else if (ssp.m_transitInfo.GetType() == routing::TransitInfo::Type::Gate)
        AddGateSubwayForSubroute(s, subroute, sp, ssp);
    }
    else if (ssp.m_transitInfo.GetVersion() == ::transit::TransitVersion::AllPublicTransport)
    {
      if (ssp.m_transitInfo.GetType() == routing::TransitInfo::Type::Edge)
        AddEdgePTForSubroute(s, subroute, sp, ssp);
      else if (ssp.m_transitInfo.GetType() == routing::TransitInfo::Type::Gate)
        AddGatePTForSubroute(s, subroute, sp, ssp);
    }
  }

  m_routeInfo.m_totalDistInMeters = sp.m_prevDistance;
  m_routeInfo.m_totalTimeInSec = static_cast<int>(ceil(sp.m_prevTime));

  bool const isValidSubroute = subroute.m_polyline.GetSize() > 1;
  if (!isValidSubroute)
    LOG(LWARNING, ("Invalid subroute. Points number =", subroute.m_polyline.GetSize()));

  return isValidSubroute;
}

template <class T>
void FillMwmTransitSubway(std::unique_ptr<TransitDisplayInfo> & mwmTransit, routing::TransitInfo const & transitInfo,
                          T mwmId)
{
  switch (transitInfo.GetType())
  {
  case routing::TransitInfo::Type::Edge:
  {
    auto const & edge = transitInfo.GetEdgeSubway();

    mwmTransit->m_stopsSubway[edge.m_stop1Id] = {};
    mwmTransit->m_stopsSubway[edge.m_stop2Id] = {};
    mwmTransit->m_linesSubway[edge.m_lineId] = {};
    for (auto const & shapeId : edge.m_shapeIds)
      mwmTransit->m_shapesSubway[shapeId] = {};
    break;
  }
  case routing::TransitInfo::Type::Transfer:
  {
    auto const & transfer = transitInfo.GetTransferSubway();
    mwmTransit->m_stopsSubway[transfer.m_stop1Id] = {};
    mwmTransit->m_stopsSubway[transfer.m_stop2Id] = {};
    break;
  }
  case routing::TransitInfo::Type::Gate:
  {
    auto const & gate = transitInfo.GetGateSubway();
    if (gate.m_featureId != kInvalidFeatureId)
    {
      auto const featureId = FeatureID(mwmId, gate.m_featureId);
      TransitFeatureInfo featureInfo;
      featureInfo.m_isGate = true;
      mwmTransit->m_features[featureId] = featureInfo;
    }
    break;
  }
  }
}

template <class T>
void FillMwmTransitPT(std::unique_ptr<TransitDisplayInfo> & mwmTransit, routing::TransitInfo const & transitInfo,
                      T mwmId)
{
  switch (transitInfo.GetType())
  {
  case routing::TransitInfo::Type::Edge:
  {
    auto const & edge = transitInfo.GetEdgePT();

    mwmTransit->m_stopsPT[edge.m_stop1Id] = {};
    mwmTransit->m_stopsPT[edge.m_stop2Id] = {};
    mwmTransit->m_linesPT[edge.m_lineId] = {};
    mwmTransit->m_shapesPT[edge.m_shapeLink.m_shapeId] = {};
    break;
  }
  case routing::TransitInfo::Type::Transfer:
  {
    auto const & transfer = transitInfo.GetTransferPT();
    mwmTransit->m_stopsPT[transfer.m_stop1Id] = {};
    mwmTransit->m_stopsPT[transfer.m_stop2Id] = {};
    break;
  }
  case routing::TransitInfo::Type::Gate:
  {
    auto const & gate = transitInfo.GetGatePT();
    if (gate.m_featureId != kInvalidFeatureId)
    {
      auto const featureId = FeatureID(mwmId, gate.m_featureId);
      TransitFeatureInfo featureInfo;
      featureInfo.m_isGate = true;
      mwmTransit->m_features[featureId] = featureInfo;
    }
    break;
  }
  }
}

void TransitRouteDisplay::CollectTransitDisplayInfo(std::vector<routing::RouteSegment> const & segments,
                                                    TransitDisplayInfos & transitDisplayInfos)
{
  for (auto const & s : segments)
  {
    if (!s.HasTransitInfo())
      continue;

    auto const mwmId = m_getMwmIdFn(s.GetSegment().GetMwmId());

    auto & mwmTransit = transitDisplayInfos[mwmId];
    if (mwmTransit == nullptr)
      mwmTransit = std::make_unique<TransitDisplayInfo>();

    routing::TransitInfo const & transitInfo = s.GetTransitInfo();

    mwmTransit->m_transitVersion = transitInfo.GetVersion();

    if (transitInfo.GetVersion() == ::transit::TransitVersion::OnlySubway)
      FillMwmTransitSubway(mwmTransit, transitInfo, mwmId);
    else if (transitInfo.GetVersion() == ::transit::TransitVersion::AllPublicTransport)
      FillMwmTransitPT(mwmTransit, transitInfo, mwmId);
  }
}

TransitMark * TransitRouteDisplay::CreateMark(m2::PointD const & pt, FeatureID const & fid)
{
  uint32_t const nextIndex = static_cast<uint32_t>(m_bmManager->GetUserMarkIds(UserMark::Type::TRANSIT).size());

  auto editSession = m_bmManager->GetEditSession();
  auto * transitMark = editSession.CreateUserMark<TransitMark>(pt);
  transitMark->SetFeatureId(fid);
  transitMark->SetIndex(nextIndex);
  return transitMark;
}

void TransitRouteDisplay::CreateTransitMarks()
{
  if (m_transitMarks.empty())
    return;

  std::vector<m2::PointF> const transferMarkerSizes = GetTransitMarkerSizes(kTransferMarkerScale, m_maxSubrouteWidth);
  std::vector<m2::PointF> const stopMarkerSizes = GetTransitMarkerSizes(kStopMarkerScale, m_maxSubrouteWidth);

  std::vector<m2::PointF> transferArrowOffsets;
  transferArrowOffsets.reserve(transferMarkerSizes.size());
  for (auto const & size : transferMarkerSizes)
    transferArrowOffsets.emplace_back(0.0f, size.y * 0.5f);

  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());

  auto editSession = m_bmManager->GetEditSession();
  for (auto const & mark : m_transitMarks)
  {
    auto transitMark = CreateMark(mark.m_point, mark.m_featureId);

    dp::TitleDecl titleDecl;
    if (mark.m_type == TransitMarkInfo::Type::Gate)
    {
      if (!mark.m_titles.empty())
      {
        TransitMark::GetDefaultTransitTitle(titleDecl);
        titleDecl.m_primaryText = mark.m_titles.front().m_text;
        titleDecl.m_primaryOffset.y = kGateTitleOffset;
        titleDecl.m_anchor = dp::Anchor::Top;
        titleDecl.m_primaryOptional = true;
        transitMark->AddTitle(titleDecl);
      }

      auto const GetSymbolName = [](std::string const & name, char const * suffix)
      { return name != kZeroIcon ? name + suffix : name; };
      df::UserPointMark::SymbolNameZoomInfo symbolNames;
      symbolNames[kSmallIconZoom] = GetSymbolName(mark.m_symbolName, "-s");
      symbolNames[kMediumIconZoom] = GetSymbolName(mark.m_symbolName, "-m");
      transitMark->SetSymbolNames(symbolNames);

      transitMark->SetPriority(UserMark::Priority::TransitGate);
    }
    else if (mark.m_type == TransitMarkInfo::Type::Transfer)
    {
      if (mark.m_titles.size() > 1)
      {
        auto titleTransitMark = CreateMark(mark.m_point, mark.m_featureId);

        TransitMark::GetDefaultTransitTitle(titleDecl);
        titleDecl.m_primaryText = mark.m_titles.front().m_text;
        titleDecl.m_primaryTextFont.m_color = df::GetColorConstant(mark.m_titles.front().m_color);
        titleDecl.m_primaryOffset.x = -kTransferTitleOffset * vs;
        titleDecl.m_anchor = dp::Anchor::Right;
        titleDecl.m_primaryOptional = false;
        titleTransitMark->AddTitle(titleDecl);

        titleDecl.m_primaryText = mark.m_titles.back().m_text;
        titleDecl.m_primaryTextFont.m_color = df::GetColorConstant(mark.m_titles.back().m_color);
        titleDecl.m_primaryOffset.x = kTransferTitleOffset * vs;
        titleDecl.m_anchor = dp::Anchor::Left;
        titleDecl.m_primaryOptional = false;
        titleTransitMark->AddTitle(titleDecl);

        titleTransitMark->SetAnchor(dp::Top);
        titleTransitMark->SetSymbolNames({{1 /* minZoom */, "transfer_arrow"}});
        titleTransitMark->SetSymbolOffsets(transferArrowOffsets);
        titleTransitMark->SetPriority(UserMark::Priority::TransitTransfer);
      }
      df::UserPointMark::ColoredSymbolZoomInfo coloredSymbol;
      for (size_t sizeIndex = 0; sizeIndex < transferMarkerSizes.size(); ++sizeIndex)
      {
        auto const zoomLevel = sizeIndex + 1;
        auto const & sz = transferMarkerSizes[sizeIndex];
        df::ColoredSymbolViewParams params;
        params.m_radiusInPixels = std::max(sz.x, sz.y) * 0.5f;
        params.m_color = dp::Color::Transparent();
        if (coloredSymbol.m_zoomInfo.empty() ||
            coloredSymbol.m_zoomInfo.rbegin()->second.m_radiusInPixels != params.m_radiusInPixels)
        {
          coloredSymbol.m_zoomInfo.insert(std::make_pair(zoomLevel, params));
        }
      }
      transitMark->SetColoredSymbols(coloredSymbol);
      transitMark->SetPriority(UserMark::Priority::TransitTransfer);
    }
    else
    {
      if (!mark.m_titles.empty())
      {
        TransitMark::GetDefaultTransitTitle(titleDecl);
        titleDecl.m_primaryText = mark.m_titles.front().m_text;
        titleDecl.m_primaryTextFont.m_color = df::GetColorConstant(mark.m_titles.front().m_color);
        titleDecl.m_primaryOffset.y = kStopTitleOffset;
        titleDecl.m_anchor = dp::Anchor::Top;
        titleDecl.m_primaryOptional = true;
        transitMark->AddTitle(titleDecl);
      }
      if (mark.m_type == TransitMarkInfo::Type::KeyStop)
      {
        df::UserPointMark::SymbolNameZoomInfo symbolNames;
        symbolNames[kSmallIconZoom] = mark.m_symbolName + "-s";
        symbolNames[kMediumIconZoom] = mark.m_symbolName + "-m";
        transitMark->SetSymbolNames(symbolNames);

        df::UserPointMark::ColoredSymbolZoomInfo coloredSymbol;
        df::ColoredSymbolViewParams params;
        params.m_color = df::GetColorConstant(mark.m_color);

        auto sz = m_symbolSizes.at(symbolNames[kSmallIconZoom]);
        params.m_radiusInPixels = std::max(sz.x, sz.y) * kGateBgScale * 0.5f;
        coloredSymbol.m_zoomInfo[kSmallIconZoom] = params;

        sz = m_symbolSizes.at(symbolNames[kMediumIconZoom]);
        params.m_radiusInPixels = std::max(sz.x, sz.y) * kGateBgScale * 0.5f;
        coloredSymbol.m_zoomInfo[kMediumIconZoom] = params;

        transitMark->SetColoredSymbols(coloredSymbol);
        transitMark->SetPriority(UserMark::Priority::TransitKeyStop);
      }
      else
      {
        df::UserPointMark::ColoredSymbolZoomInfo coloredSymbol;
        for (size_t sizeIndex = 0; sizeIndex < stopMarkerSizes.size(); ++sizeIndex)
        {
          auto const zoomLevel = sizeIndex + 1;
          auto const & sz = stopMarkerSizes[sizeIndex];
          df::ColoredSymbolViewParams params;
          params.m_radiusInPixels = std::max(sz.x, sz.y) * 0.5f;
          params.m_color = dp::Color::Transparent();
          if (coloredSymbol.m_zoomInfo.empty() ||
              coloredSymbol.m_zoomInfo.rbegin()->second.m_radiusInPixels != params.m_radiusInPixels)
          {
            coloredSymbol.m_zoomInfo.insert(std::make_pair(zoomLevel, params));
          }
        }
        transitMark->SetSymbolSizes(stopMarkerSizes);
        transitMark->SetColoredSymbols(coloredSymbol);
        transitMark->SetPriority(UserMark::Priority::TransitStop);
        transitMark->SetMinTitleZoom(kMinStopTitleZoom);
      }
    }
  }
}
