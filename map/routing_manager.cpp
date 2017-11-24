#include "routing_manager.hpp"

#include "map/chart_generator.hpp"
#include "map/routing_mark.hpp"

#include "private.h"

#include "tracking/reporter.hpp"

#include "routing/checkpoint_predictor.hpp"
#include "routing/index_router.hpp"
#include "routing/online_absent_fetcher.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/route.hpp"
#include "routing/routing_algorithm.hpp"
#include "routing/routing_helpers.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/multilang_utf8_string.hpp"

#include "platform/country_file.hpp"
#include "platform/mwm_traits.hpp"
#include "platform/platform.hpp"
#include "platform/socket.hpp"

#include "base/scope_guard.hpp"

#include "3party/Alohalytics/src/alohalytics.h"
#include "3party/jansson/myjansson.hpp"

#include <iomanip>
#include <ios>
#include <map>
#include <sstream>

using namespace routing;
using namespace std;

namespace
{
//#define SHOW_ROUTE_DEBUG_MARKS

char const kRouterTypeKey[] = "router";

double const kRouteScaleMultiplier = 1.5;
float const kStopMarkerScale = 2.0f;
float const kTransferMarkerScale = 4.0f;

string const kRoutePointsFile = "route_points.dat";

uint32_t constexpr kInvalidTransactionId = 0;

map<TransitType, string> const kTransitSymbols = {
    { TransitType::Subway, "transit_subway" },
    { TransitType::LightRail, "transit_light_rail" },
    { TransitType::Monorail, "transit_monorail" },
    { TransitType::Train, "transit_train" }
};

TransitType GetTransitType(string const & type)
{
  if (type == "subway")
    return TransitType::Subway;
  if (type == "light_rail")
    return TransitType::LightRail;
  if (type == "monorail")
    return TransitType::Monorail;

  ASSERT_EQUAL(type, "train", ());
  return TransitType::Train;
}

void FillTurnsDistancesForRendering(vector<RouteSegment> const & segments,
                                    double baseDistance, vector<double> & turns)
{
  using namespace routing::turns;
  turns.clear();
  turns.reserve(segments.size());
  for (auto const & s : segments)
  {
    auto const & t = s.GetTurn();
    CHECK_NOT_EQUAL(t.m_turn, CarDirection::Count, ());
    // We do not render some of turn directions.
    if (t.m_turn == CarDirection::None || t.m_turn == CarDirection::StartAtEndOfStreet ||
        t.m_turn == CarDirection::StayOnRoundAbout || t.m_turn == CarDirection::TakeTheExit ||
        t.m_turn == CarDirection::ReachedYourDestination)
    {
      continue;
    }
    turns.push_back(s.GetDistFromBeginningMerc() - baseDistance);
  }
}

void FillTrafficForRendering(vector<RouteSegment> const & segments,
                             vector<traffic::SpeedGroup> & traffic)
{
  traffic.clear();
  traffic.reserve(segments.size());
  for (auto const & s : segments)
    traffic.push_back(s.GetTraffic());
}

struct TransitTitle
{
  TransitTitle() = default;
  TransitTitle(string const & text, df::ColorConstant const & color) : m_text(text), m_color(color) {}

  string m_text;
  df::ColorConstant m_color;
};

struct TransitMarkInfo
{
  enum class Type
  {
    Stop,
    KeyStop,
    Transfer,
    Gate
  };
  Type m_type = Type::Stop;
  m2::PointD m_point;
  vector<TransitTitle> m_titles;
  std::string m_symbolName;
  df::ColorConstant m_color;
  FeatureID m_featureId;
};

using GetMwmIdFn = function<MwmSet::MwmId (routing::NumMwmId numMwmId)>;
void CollectTransitDisplayInfo(vector<RouteSegment> const & segments, GetMwmIdFn const & getMwmIdFn,
                               TransitDisplayInfos & transitDisplayInfos)
{
  for (auto const & s : segments)
  {
    if (!s.HasTransitInfo())
      continue;

    auto const mwmId = getMwmIdFn(s.GetSegment().GetMwmId());

    auto & mwmTransit = transitDisplayInfos[mwmId];
    if (mwmTransit == nullptr)
      mwmTransit = my::make_unique<TransitDisplayInfo>();

    TransitInfo const & transitInfo = s.GetTransitInfo();
    switch (transitInfo.GetType())
    {
      case TransitInfo::Type::Edge:
      {
        auto const & edge = transitInfo.GetEdge();

        mwmTransit->m_stops[edge.m_stop1Id] = {};
        mwmTransit->m_stops[edge.m_stop2Id] = {};
        mwmTransit->m_lines[edge.m_lineId] = {};
        for (auto const &shapeId : edge.m_shapeIds)
          mwmTransit->m_shapes[shapeId] = {};
        break;
      }
      case TransitInfo::Type::Transfer:
      {
        auto const & transfer = transitInfo.GetTransfer();
        mwmTransit->m_stops[transfer.m_stop1Id] = {};
        mwmTransit->m_stops[transfer.m_stop2Id] = {};
        break;
      }
      case TransitInfo::Type::Gate:
      {
        auto const & gate = transitInfo.GetGate();
        if (gate.m_featureId != transit::kInvalidFeatureId)
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
}

void AddTransitGateSegment(m2::PointD const & destPoint, df::ColorConstant const & color, df::Subroute & subroute)
{
  ASSERT_GREATER(subroute.m_polyline.GetSize(), 0, ());
  df::SubrouteStyle style(color, df::RoutePattern(4.0, 2.0));
  style.m_startIndex = subroute.m_polyline.GetSize() - 1;
  auto const vec = destPoint - subroute.m_polyline.Back();
  subroute.m_polyline.Add(destPoint);
  style.m_endIndex = subroute.m_polyline.GetSize() - 1;
  subroute.AddStyle(style);
}

void AddTransitPedestrianSegment(m2::PointD const & destPoint, df::Subroute & subroute)
{
  ASSERT_GREATER(subroute.m_polyline.GetSize(), 0, ());
  df::SubrouteStyle style(df::kRoutePedestrian, df::RoutePattern(4.0, 2.0));
  style.m_startIndex = subroute.m_polyline.GetSize() - 1;
  subroute.m_polyline.Add(destPoint);
  style.m_endIndex = subroute.m_polyline.GetSize() - 1;
  subroute.AddStyle(style);
}

void AddTransitShapes(std::vector<transit::ShapeId> const & shapeIds, TransitShapesInfo const & shapes,
                      df::ColorConstant const & color, bool isInverted, df::Subroute & subroute)
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

uint32_t ColorToARGB(df::ColorConstant const & colorConstant)
{
  auto const color = df::GetColorConstant(colorConstant);
  return color.GetAlpha() << 24 | color.GetRed() << 16 | color.GetGreen() << 8 | color.GetBlue();
}

void FillTransitStyleForRendering(vector<RouteSegment> const & segments, TransitReadManager & transitReadManager,
                                  GetMwmIdFn const & getMwmIdFn,
                                  RoutingManager::Callbacks::GetStringsBundleFn const & getStringsBundleFn,
                                  df::Subroute & subroute, vector<TransitMarkInfo> & transitMarks,
                                  TransitRouteInfo & routeInfo)
{
  TransitDisplayInfos transitDisplayInfos;
  CollectTransitDisplayInfo(segments, getMwmIdFn, transitDisplayInfos);

  // Read transit display info.
  if (!transitReadManager.GetTransitDisplayInfo(transitDisplayInfos))
    return;

  subroute.m_styleType = df::SubrouteStyleType::Multiple;
  subroute.m_style.clear();
  subroute.m_style.reserve(segments.size());

  df::ColorConstant lastColor;
  m2::PointD lastDir;

  df::SubrouteMarker marker;
  TransitMarkInfo transitMarkInfo;
  TransitType transitType = TransitType::Pedestrian;

  double prevDistance = routeInfo.m_totalDistInMeters;
  double prevTime = routeInfo.m_totalTimeInSec;

  bool pendingEntrance = false;

  for (auto const & s : segments)
  {
    auto const time = static_cast<int>(ceil(s.GetTimeFromBeginningSec() - prevTime));
    auto const distance = s.GetDistFromBeginningMeters() - prevDistance;
    prevDistance = s.GetDistFromBeginningMeters();
    prevTime = s.GetTimeFromBeginningSec();

    if (!s.HasTransitInfo())
    {
      routeInfo.AddStep(TransitStepInfo(TransitType::Pedestrian, distance, time));

      AddTransitPedestrianSegment(s.GetJunction().GetPoint(), subroute);
      lastColor = "";
      transitType = TransitType::Pedestrian;
      continue;
    }

    auto const mwmId = getMwmIdFn(s.GetSegment().GetMwmId());

    auto const & transitInfo = s.GetTransitInfo();
    auto const & displayInfo = *transitDisplayInfos.at(mwmId).get();

    if (transitInfo.GetType() == TransitInfo::Type::Edge)
    {
      auto const & edge = transitInfo.GetEdge();

      auto const & line = displayInfo.m_lines.at(edge.m_lineId);
      auto const currentColor = df::GetTransitColorName(line.GetColor());
      transitType = GetTransitType(line.GetType());

      routeInfo.AddStep(TransitStepInfo(transitType, distance, time,
                                        line.GetNumber(), ColorToARGB(currentColor)));

      auto const & stop1 = displayInfo.m_stops.at(edge.m_stop1Id);
      auto const & stop2 = displayInfo.m_stops.at(edge.m_stop2Id);
      bool const isTransfer1 = stop1.GetTransferId() != transit::kInvalidTransferId;
      bool const isTransfer2 = stop2.GetTransferId() != transit::kInvalidTransferId;

      marker.m_distance = prevDistance;
      marker.m_scale = kStopMarkerScale;
      marker.m_innerColor = currentColor;
      if (isTransfer1)
      {
        auto const & transferData = displayInfo.m_transfers.at(stop1.GetTransferId());
        marker.m_position = transferData.GetPoint();
      }
      else
      {
        marker.m_position = stop1.GetPoint();
      }
      transitMarkInfo.m_point = marker.m_position;

      if (pendingEntrance)
      {
        transitMarkInfo.m_type = TransitMarkInfo::Type::KeyStop;
        transitMarkInfo.m_symbolName = kTransitSymbols.at(transitType);
        transitMarkInfo.m_color = currentColor;
        AddTransitGateSegment(marker.m_position, currentColor, subroute);
        pendingEntrance = false;
      }

      auto const id1 = isTransfer1 ? stop1.GetTransferId() : stop1.GetId();
      auto const id2 = isTransfer2 ? stop2.GetTransferId() : stop2.GetId();
      bool const isInverted = id1 > id2;

      AddTransitShapes(edge.m_shapeIds, displayInfo.m_shapes, currentColor, isInverted, subroute);

      ASSERT_GREATER(subroute.m_polyline.GetSize(), 1, ());
      auto const & p1 = *(subroute.m_polyline.End() - 2);
      auto const & p2 = *(subroute.m_polyline.End() - 1);
      m2::PointD currentDir = (p2 - p1).Normalize();

      if (lastColor != currentColor)
      {
        if (!lastColor.empty())
        {
          marker.m_scale = kTransferMarkerScale;
          transitMarkInfo.m_type = TransitMarkInfo::Type::Transfer;
        }
        marker.m_colors.push_back(currentColor);

        if (stop1.GetFeatureId() != transit::kInvalidFeatureId)
        {
          auto const fid = FeatureID(mwmId, stop1.GetFeatureId());
          transitMarkInfo.m_featureId = fid;
          transitMarkInfo.m_titles.emplace_back(displayInfo.m_features.at(fid).m_title,
                                                df::GetTransitTextColorName(line.GetColor()));
        }
      }
      lastColor = currentColor;

      if (marker.m_colors.size() > 1)
      {
        marker.m_innerColor = df::kTransitOutlineColor;
        marker.m_up = (currentDir - lastDir).Normalize();
        if (m2::CrossProduct(marker.m_up, -lastDir) < 0)
          marker.m_up = -marker.m_up;
      }

      subroute.m_markers.push_back(marker);
      marker = df::SubrouteMarker();

      transitMarks.push_back(transitMarkInfo);
      transitMarkInfo = TransitMarkInfo();

      lastDir = currentDir;

      marker.m_distance = s.GetDistFromBeginningMeters();
      marker.m_scale = kStopMarkerScale;
      marker.m_innerColor = currentColor;
      marker.m_colors.push_back(currentColor);

      if (isTransfer2)
      {
        auto const & transferData = displayInfo.m_transfers.at(stop2.GetTransferId());
        marker.m_position = transferData.GetPoint();
      }
      else
      {
        marker.m_position = stop2.GetPoint();
      }

      transitMarkInfo.m_point = marker.m_position;
      if (stop2.GetFeatureId() != transit::kInvalidFeatureId)
      {
        auto const fid = FeatureID(mwmId, stop2.GetFeatureId());
        transitMarkInfo.m_featureId = fid;
        transitMarkInfo.m_titles.push_back(TransitTitle(displayInfo.m_features.at(fid).m_title,
                                                        df::GetTransitTextColorName(line.GetColor())));
      }
    }
    else if (transitInfo.GetType() == TransitInfo::Type::Gate)
    {
      auto const & gate = transitInfo.GetGate();
      if (!lastColor.empty())
      {
        routeInfo.AddStep(TransitStepInfo(TransitType::Pedestrian, distance, time));

        AddTransitGateSegment(s.GetJunction().GetPoint(), lastColor, subroute);

        subroute.m_markers.push_back(marker);
        marker = df::SubrouteMarker();

        transitMarkInfo.m_type = TransitMarkInfo::Type::KeyStop;
        transitMarkInfo.m_symbolName = kTransitSymbols.at(transitType);
        transitMarkInfo.m_color = lastColor;
        transitMarks.push_back(transitMarkInfo);
        transitMarkInfo = TransitMarkInfo();
      }
      else
      {
        pendingEntrance = true;
      }

      auto gateMarkInfo = TransitMarkInfo();
      gateMarkInfo.m_point = pendingEntrance ? subroute.m_polyline.Back() : s.GetJunction().GetPoint();
      gateMarkInfo.m_type = TransitMarkInfo::Type::Gate;
      if (gate.m_featureId != transit::kInvalidFeatureId)
      {
        auto const fid = FeatureID(mwmId, gate.m_featureId);
        auto const & featureInfo = displayInfo.m_features.at(fid);
        auto symbolName = featureInfo.m_gateSymbolName;
        if (strings::EndsWith(symbolName, "-s") ||
            strings::EndsWith(symbolName, "-m") ||
            strings::EndsWith(symbolName, "-l"))
        {
          symbolName = symbolName.substr(0, symbolName.length() - 2);
        }

        gateMarkInfo.m_featureId = fid;
        gateMarkInfo.m_symbolName = symbolName;
        string title = pendingEntrance ? "entrance" : "exit";//getStringsBundleFn().GetString(pendingEntrance ? "entrance" : "exit");
        gateMarkInfo.m_titles.push_back(TransitTitle(title, df::GetTransitTextColorName("default")));
      }

      transitMarks.push_back(gateMarkInfo);
    }
  }

  routeInfo.m_totalDistInMeters = prevDistance;
  routeInfo.m_totalTimeInSec = static_cast<int>(ceil(prevTime));
}

vector<m2::PointF> GetTransitMarkerSizes(float markerScale)
{
  vector<m2::PointF> markerSizes;
  markerSizes.reserve(df::kRouteHalfWidthInPixelTransit.size());
  for (auto const halfWidth : df::kRouteHalfWidthInPixelTransit)
  {
    float const d = 2 * halfWidth * markerScale;
    markerSizes.push_back(m2::PointF(d, d));
  }
  return markerSizes;
}

void CreateTransitMarks(vector<TransitMarkInfo> const & transitMarks, std::map<std::string, m2::PointF> symbolSizes,
                        UserMarksController & marksController)
{
  static vector<m2::PointF> const kTransferMarkerSizes = GetTransitMarkerSizes(kTransferMarkerScale);
  static vector<m2::PointF> const kStopMarkerSizes = GetTransitMarkerSizes(kStopMarkerScale);

  static const int kSmallIconZoom = 1;
  static const int kMediumIconZoom = 10;
  static const int kLargeIconZoom = 15;

  static const float kGateBgScale = 1.2f;

  auto const vs = df::VisualParams::Instance().GetVisualScale();
  for (size_t i = 0; i < transitMarks.size(); ++i)
  {
    auto const &mark = transitMarks[i];

    auto userMark = marksController.CreateUserMark(mark.m_point);
    ASSERT(dynamic_cast<TransitMark *>(userMark) != nullptr, ());
    auto transitMark = static_cast<TransitMark *>(userMark);
    dp::TitleDecl titleDecl;

    transitMark->SetFeatureId(mark.m_featureId);

    if (mark.m_type == TransitMarkInfo::Type::Gate)
    {
      if (!mark.m_titles.empty())
      {
        TransitMark::GetDefaultTransitTitle(titleDecl);
        titleDecl.m_primaryText = mark.m_titles.front().m_text;
        titleDecl.m_anchor = dp::Anchor::Top;
        titleDecl.m_primaryOptional = true;
        transitMark->AddTitle(titleDecl);
      }
      df::UserPointMark::SymbolNameZoomInfo symbolNames;
      symbolNames[kSmallIconZoom] = mark.m_symbolName + "-s";
      symbolNames[kMediumIconZoom] = mark.m_symbolName + "-m";
      symbolNames[kLargeIconZoom] = mark.m_symbolName + "-l";
      transitMark->SetSymbolNames(symbolNames);
      transitMark->SetPriority(UserMark::Priority::Transit_Gate);
    }
    else if (mark.m_type == TransitMarkInfo::Type::Transfer)
    {
      if (mark.m_titles.size() > 1)
      {
        TransitMark::GetDefaultTransitTitle(titleDecl);
        titleDecl.m_primaryText = mark.m_titles.front().m_text;
        titleDecl.m_primaryTextFont.m_color = df::GetColorConstant(mark.m_titles.front().m_color);
        titleDecl.m_anchor = dp::Anchor::Bottom;
        titleDecl.m_primaryOptional = true;
        transitMark->AddTitle(titleDecl);

        titleDecl.m_primaryText = mark.m_titles.back().m_text;
        titleDecl.m_primaryTextFont.m_color = df::GetColorConstant(mark.m_titles.back().m_color);
        titleDecl.m_anchor = dp::Anchor::Top;
        titleDecl.m_primaryOptional = true;
        transitMark->AddTitle(titleDecl);
      }
      df::UserPointMark::ColoredSymbolZoomInfo coloredSymbol;
      for (size_t sizeIndex = 0; sizeIndex < kTransferMarkerSizes.size(); ++sizeIndex)
      {
        auto const zoomLevel = sizeIndex + 1;
        auto const & sz = kTransferMarkerSizes[sizeIndex];
        df::ColoredSymbolViewParams params;
        params.m_radiusInPixels = static_cast<float>(max(sz.x, sz.y) * vs / 2.0);
        params.m_color = dp::Color::Transparent();
        if (coloredSymbol.empty() || coloredSymbol.rbegin()->second.m_radiusInPixels != params.m_radiusInPixels)
          coloredSymbol.insert(make_pair(zoomLevel, params));
      }
      transitMark->SetColoredSymbols(coloredSymbol);
      transitMark->SetPriority(UserMark::Priority::Transit_Transfer);
    }
    else
    {
      if (!mark.m_titles.empty())
      {
        TransitMark::GetDefaultTransitTitle(titleDecl);
        titleDecl.m_primaryText = mark.m_titles.front().m_text;
        titleDecl.m_primaryTextFont.m_color = df::GetColorConstant(mark.m_titles.front().m_color);
        titleDecl.m_anchor = dp::Anchor::Top;
        titleDecl.m_primaryOptional = true;
        transitMark->AddTitle(titleDecl);
      }
      if (mark.m_type == TransitMarkInfo::Type::KeyStop)
      {
        df::UserPointMark::SymbolNameZoomInfo symbolNames;
        symbolNames[kSmallIconZoom] = mark.m_symbolName + "-s";
        symbolNames[kMediumIconZoom] = mark.m_symbolName + "-m";
        symbolNames[kLargeIconZoom] = mark.m_symbolName + "-l";
        transitMark->SetSymbolNames(symbolNames);

        df::UserPointMark::ColoredSymbolZoomInfo coloredSymbol;
        df::ColoredSymbolViewParams params;
        params.m_color = df::GetColorConstant(mark.m_color);

        auto sz = symbolSizes[symbolNames[kSmallIconZoom]];
        params.m_radiusInPixels = static_cast<float>(max(sz.x, sz.y) * kGateBgScale / 2.0);
        coloredSymbol[kSmallIconZoom] = params;

        sz = symbolSizes[symbolNames[kMediumIconZoom]];
        params.m_radiusInPixels = static_cast<float>(max(sz.x, sz.y) * kGateBgScale / 2.0);
        coloredSymbol[kMediumIconZoom] = params;

        sz = symbolSizes[symbolNames[kLargeIconZoom]];
        params.m_radiusInPixels = static_cast<float>(max(sz.x, sz.y) * kGateBgScale / 2.0);
        coloredSymbol[kLargeIconZoom] = params;

        transitMark->SetColoredSymbols(coloredSymbol);
        transitMark->SetPriority(UserMark::Priority::Transit_KeyStop);
      }
      else
      {
        df::UserPointMark::ColoredSymbolZoomInfo coloredSymbol;
        for (size_t sizeIndex = 0; sizeIndex < kStopMarkerSizes.size(); ++sizeIndex)
        {
          auto const zoomLevel = sizeIndex + 1;
          auto const & sz = kStopMarkerSizes[sizeIndex];
          df::ColoredSymbolViewParams params;
          params.m_radiusInPixels = static_cast<float>(max(sz.x, sz.y) * vs / 2.0);
          params.m_color = dp::Color::Transparent();
          if (coloredSymbol.empty() || coloredSymbol.rbegin()->second.m_radiusInPixels != params.m_radiusInPixels)
            coloredSymbol.insert(make_pair(zoomLevel, params));
        }
        transitMark->SetMinZoom(16);
        transitMark->SetColoredSymbols(coloredSymbol);
        transitMark->SetPriority(UserMark::Priority::Transit_Stop);
      }
    }
  }
}

RouteMarkData GetLastPassedPoint(BookmarkManager * bmManager, vector<RouteMarkData> const & points)
{
  ASSERT_GREATER_OR_EQUAL(points.size(), 2, ());
  ASSERT(points[0].m_pointType == RouteMarkType::Start, ());
  RouteMarkData data = points[0];

  for (int i = static_cast<int>(points.size()) - 1; i >= 0; i--)
  {
    if (points[i].m_isPassed)
    {
      data = points[i];
      break;
    }
  }

  // Last passed point will be considered as start point.
  data.m_pointType = RouteMarkType::Start;
  data.m_intermediateIndex = 0;
  if (data.m_isMyPosition)
  {
    data.m_position = bmManager->MyPositionMark()->GetPivot();
    data.m_isMyPosition = false;
  }

  return data;
}

void SerializeRoutePoint(json_t * node, RouteMarkData const & data)
{
  ASSERT(node != nullptr, ());
  ToJSONObject(*node, "type", static_cast<int>(data.m_pointType));
  ToJSONObject(*node, "title", data.m_title);
  ToJSONObject(*node, "subtitle", data.m_subTitle);
  ToJSONObject(*node, "x", data.m_position.x);
  ToJSONObject(*node, "y", data.m_position.y);
}

RouteMarkData DeserializeRoutePoint(json_t * node)
{
  ASSERT(node != nullptr, ());
  RouteMarkData data;

  int type = 0;
  FromJSONObject(node, "type", type);
  data.m_pointType = static_cast<RouteMarkType>(type);

  FromJSONObject(node, "title", data.m_title);
  FromJSONObject(node, "subtitle", data.m_subTitle);

  FromJSONObject(node, "x", data.m_position.x);
  FromJSONObject(node, "y", data.m_position.y);

  return data;
}

string SerializeRoutePoints(BookmarkManager * bmManager, vector<RouteMarkData> const & points)
{
  ASSERT_GREATER_OR_EQUAL(points.size(), 2, ());
  auto pointsNode = my::NewJSONArray();

  // Save last passed point. It will be used on points loading if my position
  // isn't determined.
  auto lastPassedPoint = GetLastPassedPoint(bmManager, points);
  auto lastPassedNode = my::NewJSONObject();
  SerializeRoutePoint(lastPassedNode.get(), lastPassedPoint);
  json_array_append_new(pointsNode.get(), lastPassedNode.release());

  for (auto const & p : points)
  {
    // Here we skip passed points and the start point.
    if (p.m_isPassed || p.m_pointType == RouteMarkType::Start)
      continue;

    auto pointNode = my::NewJSONObject();
    SerializeRoutePoint(pointNode.get(), p);
    json_array_append_new(pointsNode.get(), pointNode.release());
  }
  unique_ptr<char, JSONFreeDeleter> buffer(
    json_dumps(pointsNode.get(), JSON_COMPACT | JSON_ENSURE_ASCII));
  return string(buffer.get());
}

vector<RouteMarkData> DeserializeRoutePoints(string const & data)
{
  my::Json root(data.c_str());

  if (root.get() == nullptr || !json_is_array(root.get()))
    return {};

  size_t const sz = json_array_size(root.get());
  if (sz == 0)
    return {};

  vector<RouteMarkData> result;
  result.reserve(sz);
  for (size_t i = 0; i < sz; ++i)
  {
    auto pointNode = json_array_get(root.get(), i);
    if (pointNode == nullptr)
      continue;

    auto point = DeserializeRoutePoint(pointNode);
    if (point.m_position.EqualDxDy(m2::PointD::Zero(), 1e-7))
      continue;

    result.push_back(move(point));
  }

  if (result.size() < 2)
    return {};

  return result;
}

VehicleType GetVehicleType(RouterType routerType)
{
  switch (routerType)
  {
  case RouterType::Pedestrian: return VehicleType::Pedestrian;
  case RouterType::Bicycle: return VehicleType::Bicycle;
  case RouterType::Vehicle:
  case RouterType::Taxi: return VehicleType::Car;
  case RouterType::Transit: return VehicleType::Transit;
  case RouterType::Count: CHECK(false, ("Invalid type", routerType)); return VehicleType::Count;
  }
}
}  // namespace

namespace marketing
{
char const * const kRoutingCalculatingRoute = "Routing_CalculatingRoute";
}  // namespace marketing

TransitStepInfo::TransitStepInfo(TransitType type, double distance, int time,
                                 std::string const & number, uint32_t color,
                                 int intermediateIndex)
  : m_type(type)
  , m_distanceInMeters(distance)
  , m_timeInSec(time)
  , m_number(number)
  , m_colorARGB(color)
  , m_intermediateIndex(intermediateIndex)
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
    m_steps.back().m_distanceInMeters += step.m_distanceInMeters;
    m_steps.back().m_timeInSec += step.m_timeInSec;
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
  {
    FormatDistance(step.m_distanceInMeters, step.m_distanceStr, step.m_distanceUnitsSuffix);
  }
  FormatDistance(m_totalDistInMeters, m_totalDistanceStr, m_totalDistanceUnitsSuffix);
  FormatDistance(m_totalPedestrianDistInMeters, m_totalPedestrianDistanceStr,
                 m_totalPedestrianUnitsSuffix);
}

RoutingManager::RoutingManager(Callbacks && callbacks, Delegate & delegate)
  : m_callbacks(move(callbacks))
  , m_delegate(delegate)
  , m_trackingReporter(platform::CreateSocket(), TRACKING_REALTIME_HOST, TRACKING_REALTIME_PORT,
                       tracking::Reporter::kPushDelayMs)
  , m_transitReadManager(m_callbacks.m_indexGetter(), m_callbacks.m_readFeaturesFn)
{
  auto const routingStatisticsFn = [](map<string, string> const & statistics) {
    alohalytics::LogEvent("Routing_CalculatingRoute", statistics);
    GetPlatform().GetMarketingService().SendMarketingEvent(marketing::kRoutingCalculatingRoute, {});
  };

  m_routingSession.Init(routingStatisticsFn, [this](m2::PointD const & pt)
  {
#ifdef SHOW_ROUTE_DEBUG_MARKS
    if (m_bmManager == nullptr)
      return;
    auto & controller = m_bmManager->GetUserMarksController(UserMark::Type::DEBUG_MARK);
    controller.SetIsVisible(true);
    controller.SetIsDrawable(true);
    controller.CreateUserMark(pt);
    controller.NotifyChanges();
#endif
  });

  m_routingSession.SetReadyCallbacks(
      [this](Route const & route, IRouter::ResultCode code) { OnBuildRouteReady(route, code); },
      [this](Route const & route, IRouter::ResultCode code) { OnRebuildRouteReady(route, code); });

  m_routingSession.SetCheckpointCallback([this](size_t passedCheckpointIdx)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, passedCheckpointIdx]()
    {
      size_t const pointsCount = GetRoutePointsCount();

      // TODO(@bykoianko). Since routing system may invoke callbacks from different threads and here
      // we have to use gui thread, ASSERT is not correct. Uncomment it and delete condition after
      // refactoring of threads usage in routing system.
      //ASSERT_LESS(passedCheckpointIdx, pointsCount, ());
      if (passedCheckpointIdx >= pointsCount)
        return;

      if (passedCheckpointIdx == 0)
        OnRoutePointPassed(RouteMarkType::Start, 0);
      else if (passedCheckpointIdx + 1 == pointsCount)
        OnRoutePointPassed(RouteMarkType::Finish, 0);
      else
        OnRoutePointPassed(RouteMarkType::Intermediate, passedCheckpointIdx - 1);
    });
  });
}

void RoutingManager::SetBookmarkManager(BookmarkManager * bmManager)
{
  m_bmManager = bmManager;
}

void RoutingManager::OnBuildRouteReady(Route const & route, IRouter::ResultCode code)
{
  // Hide preview.
  HidePreviewSegments();

  storage::TCountriesVec absentCountries;
  if (code == IRouter::NoError)
  {
    InsertRoute(route);
    m_drapeEngine.SafeCall(&df::DrapeEngine::StopLocationFollow);

    // Validate route (in case of bicycle routing it can be invalid).
    ASSERT(route.IsValid(), ());
    if (route.IsValid())
    {
      m2::RectD routeRect = route.GetPoly().GetLimitRect();
      routeRect.Scale(kRouteScaleMultiplier);
      m_drapeEngine.SafeCall(&df::DrapeEngine::SetModelViewRect, routeRect,
                             true /* applyRotation */, -1 /* zoom */, true /* isAnim */);
    }
  }
  else
  {
    absentCountries.assign(route.GetAbsentCountries().begin(), route.GetAbsentCountries().end());

    if (code != IRouter::NeedMoreMaps)
      RemoveRoute(true /* deactivateFollowing */);
  }
  CallRouteBuilded(code, absentCountries);
}

void RoutingManager::OnRebuildRouteReady(Route const & route, IRouter::ResultCode code)
{
  // Hide preview.
  HidePreviewSegments();

  if (code != IRouter::NoError)
    return;

  InsertRoute(route);
  CallRouteBuilded(code, storage::TCountriesVec());
}

void RoutingManager::OnRoutePointPassed(RouteMarkType type, size_t intermediateIndex)
{
  // Remove route point.
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(m_bmManager->GetUserMarksController(UserMark::Type::ROUTING));
  routePoints.PassRoutePoint(type, intermediateIndex);
  routePoints.NotifyChanges();

  if (type == RouteMarkType::Finish)
    RemoveRoute(false /* deactivateFollowing */);
}

RouterType RoutingManager::GetBestRouter(m2::PointD const & startPoint,
                                         m2::PointD const & finalPoint) const
{
  // todo Implement something more sophisticated here (or delete the method).
  return GetLastUsedRouter();
}

RouterType RoutingManager::GetLastUsedRouter() const
{
  string routerTypeStr;
  if (!settings::Get(kRouterTypeKey, routerTypeStr))
    return RouterType::Vehicle;

  auto const routerType = FromString(routerTypeStr);

  switch (routerType)
  {
  case RouterType::Pedestrian:
  case RouterType::Bicycle:
  case RouterType::Transit: return routerType;
  default: return RouterType::Vehicle;
  }
}

void RoutingManager::SetRouterImpl(RouterType type)
{
  auto const indexGetterFn = m_callbacks.m_indexGetter;
  CHECK(indexGetterFn, ("Type:", type));

  VehicleType const vehicleType = GetVehicleType(type);

  m_loadAltitudes = vehicleType != VehicleType::Car;

  auto const countryFileGetter = [this](m2::PointD const & p) -> string {
    // TODO (@gorshenin): fix CountryInfoGetter to return CountryFile
    // instances instead of plain strings.
    return m_callbacks.m_countryInfoGetter().GetRegionCountryId(p);
  };

  auto numMwmIds = make_shared<NumMwmIds>();
  m_delegate.RegisterCountryFilesOnRoute(numMwmIds);

  auto & index = m_callbacks.m_indexGetter();

  auto localFileChecker = [this](string const & countryFile) -> bool {
    MwmSet::MwmId const mwmId = m_callbacks.m_indexGetter().GetMwmIdByCountryFile(
      platform::CountryFile(countryFile));
    if (!mwmId.IsAlive())
      return false;

    return version::MwmTraits(mwmId.GetInfo()->m_version).HasRoutingIndex();
  };

  auto const getMwmRectByName = [this](string const & countryId) -> m2::RectD {
    return m_callbacks.m_countryInfoGetter().GetLimitRectForLeaf(countryId);
  };

  auto fetcher = make_unique<OnlineAbsentCountriesFetcher>(countryFileGetter, localFileChecker);
  auto router = make_unique<IndexRouter>(vehicleType, m_loadAltitudes, m_callbacks.m_countryParentNameGetterFn,
                                         countryFileGetter, getMwmRectByName, numMwmIds,
                                         MakeNumMwmTree(*numMwmIds, m_callbacks.m_countryInfoGetter()),
                                         m_routingSession, index);

  m_routingSession.SetRoutingSettings(GetRoutingSettings(vehicleType));
  m_routingSession.SetRouter(move(router), move(fetcher));
  m_currentRouterType = type;
}

void RoutingManager::RemoveRoute(bool deactivateFollowing)
{
  auto & marksController = m_bmManager->GetUserMarksController(UserMark::Type::TRANSIT);
  marksController.Clear();
  marksController.NotifyChanges();

  if (deactivateFollowing)
    SetPointsFollowingMode(false /* enabled */);

  if (deactivateFollowing)
  {
    // Remove all subroutes.
    m_drapeEngine.SafeCall(&df::DrapeEngine::RemoveSubroute,
                           dp::DrapeID(), true /* deactivateFollowing */);
  }
  else
  {
    auto const subroutes = GetSubrouteIds();
    df::DrapeEngineLockGuard lock(m_drapeEngine);
    if (lock)
    {
      for (auto const & subrouteId : subroutes)
        lock.Get()->RemoveSubroute(subrouteId, false /* deactivateFollowing */);
    }
  }

  {
    lock_guard<mutex> lock(m_drapeSubroutesMutex);
    m_drapeSubroutes.clear();
    m_transitRouteInfo = TransitRouteInfo();
  }
}

void RoutingManager::InsertRoute(Route const & route)
{
  if (!m_drapeEngine)
    return;

  // TODO: Now we always update whole route, so we need to remove previous one.
  RemoveRoute(false /* deactivateFollowing */);

  vector<RouteSegment> segments;
  vector<m2::PointD> points;
  double distance = 0.0;
  TransitRouteInfo transitRouteInfo;
  auto const subroutesCount = route.GetSubrouteCount();
  for (size_t subrouteIndex = route.GetCurrentSubrouteIdx(); subrouteIndex < subroutesCount; ++subrouteIndex)
  {
    route.GetSubrouteInfo(subrouteIndex, segments);

    // Fill points.
    double const currentBaseDistance = distance;
    auto subroute = make_unique_dp<df::Subroute>();
    subroute->m_baseDistance = currentBaseDistance;
    subroute->m_baseDepthIndex = static_cast<double>(subroutesCount - subrouteIndex - 1);
    auto const startPt = route.GetSubrouteAttrs(subrouteIndex).GetStart().GetPoint();
    if (m_currentRouterType != RouterType::Transit)
    {
      points.clear();
      points.reserve(segments.size() + 1);
      points.push_back(startPt);
      for (auto const & s : segments)
        points.push_back(s.GetJunction().GetPoint());
      if (points.size() < 2)
      {
        LOG(LWARNING, ("Invalid subroute. Points number =", points.size()));
        continue;
      }
      subroute->m_polyline = m2::PolylineD(points);
    }
    else
    {
      subroute->m_polyline.Add(startPt);
    }
    distance = segments.back().GetDistFromBeginningMerc();

    switch (m_currentRouterType)
    {
      case RouterType::Vehicle:
      case RouterType::Taxi:
        subroute->m_routeType = m_currentRouterType == RouterType::Vehicle ?
                                df::RouteType::Car : df::RouteType::Taxi;
        subroute->AddStyle(df::SubrouteStyle(df::kRouteColor, df::kRouteOutlineColor));
        FillTrafficForRendering(segments, subroute->m_traffic);
        FillTurnsDistancesForRendering(segments, subroute->m_baseDistance,
                                       subroute->m_turns);
        break;
      case RouterType::Transit:
        {
          subroute->m_routeType = df::RouteType::Transit;

          auto numMwmIds = make_shared<NumMwmIds>();
          m_delegate.RegisterCountryFilesOnRoute(numMwmIds);
          auto getMwmIdFn = [this, &numMwmIds](routing::NumMwmId numMwmId)
          {
            return m_callbacks.m_indexGetter().GetMwmIdByCountryFile(numMwmIds->GetFile(numMwmId));
          };

          vector<TransitMarkInfo> transitMarks;
          if (subrouteIndex > 0)
          {
            TransitStepInfo step;
            step.m_type = TransitType::IntermediatePoint;
            step.m_intermediateIndex = static_cast<int>(subrouteIndex - 1);
            transitRouteInfo.AddStep(step);
          }

          FillTransitStyleForRendering(segments, m_transitReadManager, getMwmIdFn, m_callbacks.m_stringsBundleGetter,
                                       *subroute.get(), transitMarks, transitRouteInfo);

          if (subroute->m_polyline.GetSize() < 2)
          {
            LOG(LWARNING, ("Invalid transit subroute. Points number =", subroute->m_polyline.GetSize()));
            continue;
          }

          auto & marksController = m_bmManager->GetUserMarksController(UserMark::Type::TRANSIT);
          CreateTransitMarks(transitMarks, m_transitSymbolSizes, marksController);
          marksController.NotifyChanges();
          break;
        }
      case RouterType::Pedestrian:
        subroute->m_routeType = df::RouteType::Pedestrian;
        subroute->AddStyle(df::SubrouteStyle(df::kRoutePedestrian, df::RoutePattern(4.0, 2.0)));
        break;
      case RouterType::Bicycle:
        subroute->m_routeType = df::RouteType::Bicycle;
        subroute->AddStyle(df::SubrouteStyle(df::kRouteBicycle, df::RoutePattern(8.0, 2.0)));
        FillTurnsDistancesForRendering(segments, subroute->m_baseDistance,
                                       subroute->m_turns);
        break;
      default: ASSERT(false, ("Unknown router type"));
    }

    auto const subrouteId = m_drapeEngine.SafeCallWithResult(&df::DrapeEngine::AddSubroute,
                                                             df::SubrouteConstPtr(subroute.release()));

    // TODO: we will send subrouteId to routing subsystem when we can partly update route.
    //route.SetSubrouteUid(subrouteIndex, static_cast<SubrouteUid>(subrouteId));

    lock_guard<mutex> lock(m_drapeSubroutesMutex);
    m_drapeSubroutes.push_back(subrouteId);
    transitRouteInfo.UpdateDistanceStrings();
    m_transitRouteInfo = transitRouteInfo;
  }
}

void RoutingManager::FollowRoute()
{
  if (!m_routingSession.EnableFollowMode())
    return;

  m_delegate.OnRouteFollow(m_currentRouterType);

  HideRoutePoint(RouteMarkType::Start);
  SetPointsFollowingMode(true /* enabled */);

  CancelRecommendation(Recommendation::RebuildAfterPointsLoading);
}

void RoutingManager::CloseRouting(bool removeRoutePoints)
{
  // Hide preview.
  HidePreviewSegments();
  
  if (m_routingSession.IsBuilt())
    m_routingSession.EmitCloseRoutingEvent();
  m_routingSession.Reset();
  RemoveRoute(true /* deactivateFollowing */);

  if (removeRoutePoints)
  {
    auto & controller = m_bmManager->GetUserMarksController(UserMark::Type::ROUTING);
    controller.Clear();
    controller.NotifyChanges();

    CancelRecommendation(Recommendation::RebuildAfterPointsLoading);
  }
}

void RoutingManager::SetLastUsedRouter(RouterType type)
{
  settings::Set(kRouterTypeKey, ToString(type));
}

void RoutingManager::HideRoutePoint(RouteMarkType type, size_t intermediateIndex)
{
  RoutePointsLayout routePoints(m_bmManager->GetUserMarksController(UserMark::Type::ROUTING));
  RouteMarkPoint * mark = routePoints.GetRoutePoint(type, intermediateIndex);
  if (mark != nullptr)
  {
    mark->SetIsVisible(false);
    routePoints.NotifyChanges();
  }
}

bool RoutingManager::IsMyPosition(RouteMarkType type, size_t intermediateIndex)
{
  RoutePointsLayout routePoints(m_bmManager->GetUserMarksController(UserMark::Type::ROUTING));
  RouteMarkPoint * mark = routePoints.GetRoutePoint(type, intermediateIndex);
  return mark != nullptr ? mark->IsMyPosition() : false;
}

vector<RouteMarkData> RoutingManager::GetRoutePoints() const
{
  vector<RouteMarkData> result;
  RoutePointsLayout routePoints(m_bmManager->GetUserMarksController(UserMark::Type::ROUTING));
  for (auto const & p : routePoints.GetRoutePoints())
    result.push_back(p->GetMarkData());
  return result;
}

size_t RoutingManager::GetRoutePointsCount() const
{
  RoutePointsLayout routePoints(m_bmManager->GetUserMarksController(UserMark::Type::ROUTING));
  return routePoints.GetRoutePointsCount();
}

bool RoutingManager::CouldAddIntermediatePoint() const
{
  if (!IsRoutingActive())
    return false;

  auto const & controller = m_bmManager->GetUserMarksController(UserMark::Type::ROUTING);
  return controller.GetUserMarkCount() < RoutePointsLayout::kMaxIntermediatePointsCount + 2;
}

void RoutingManager::AddRoutePoint(RouteMarkData && markData)
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(m_bmManager->GetUserMarksController(UserMark::Type::ROUTING));

  // Always replace start and finish points.
  if (markData.m_pointType == RouteMarkType::Start || markData.m_pointType == RouteMarkType::Finish)
    routePoints.RemoveRoutePoint(markData.m_pointType);

  if (markData.m_isMyPosition)
  {
    RouteMarkPoint * mark = routePoints.GetMyPositionPoint();
    if (mark != nullptr)
      routePoints.RemoveRoutePoint(mark->GetRoutePointType(), mark->GetIntermediateIndex());
  }

  markData.m_isVisible = !markData.m_isMyPosition;
  routePoints.AddRoutePoint(move(markData));
  ReorderIntermediatePoints();
  routePoints.NotifyChanges();
}

void RoutingManager::RemoveRoutePoint(RouteMarkType type, size_t intermediateIndex)
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(m_bmManager->GetUserMarksController(UserMark::Type::ROUTING));
  routePoints.RemoveRoutePoint(type, intermediateIndex);
  routePoints.NotifyChanges();
}

void RoutingManager::RemoveRoutePoints()
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(m_bmManager->GetUserMarksController(UserMark::Type::ROUTING));
  routePoints.RemoveRoutePoints();
  routePoints.NotifyChanges();
}

void RoutingManager::RemoveIntermediateRoutePoints()
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(m_bmManager->GetUserMarksController(UserMark::Type::ROUTING));
  routePoints.RemoveIntermediateRoutePoints();
  routePoints.NotifyChanges();
}

void RoutingManager::MoveRoutePoint(RouteMarkType currentType, size_t currentIntermediateIndex,
                                    RouteMarkType targetType, size_t targetIntermediateIndex)
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(m_bmManager->GetUserMarksController(UserMark::Type::ROUTING));
  routePoints.MoveRoutePoint(currentType, currentIntermediateIndex,
                             targetType, targetIntermediateIndex);
  routePoints.NotifyChanges();
}

void RoutingManager::MoveRoutePoint(size_t currentIndex, size_t targetIndex)
{
  ASSERT(m_bmManager != nullptr, ());

  RoutePointsLayout routePoints(m_bmManager->GetUserMarksController(UserMark::Type::ROUTING));
  size_t const sz = routePoints.GetRoutePointsCount();
  auto const convertIndex = [sz](RouteMarkType & type, size_t & index) {
    if (index == 0)
    {
      type = RouteMarkType::Start;
      index = 0;
    }
    else if (index + 1 == sz)
    {
      type = RouteMarkType::Finish;
      index = 0;
    }
    else
    {
      type = RouteMarkType::Intermediate;
      --index;
    }
  };
  RouteMarkType currentType;
  RouteMarkType targetType;

  convertIndex(currentType, currentIndex);
  convertIndex(targetType, targetIndex);

  routePoints.MoveRoutePoint(currentType, currentIndex, targetType, targetIndex);
  routePoints.NotifyChanges();
}

void RoutingManager::SetPointsFollowingMode(bool enabled)
{
  ASSERT(m_bmManager != nullptr, ());
  RoutePointsLayout routePoints(m_bmManager->GetUserMarksController(UserMark::Type::ROUTING));
  routePoints.SetFollowingMode(enabled);
  routePoints.NotifyChanges();
}

void RoutingManager::ReorderIntermediatePoints()
{
  vector<RouteMarkPoint *> prevPoints;
  vector<m2::PointD> prevPositions;
  prevPoints.reserve(RoutePointsLayout::kMaxIntermediatePointsCount);
  prevPositions.reserve(RoutePointsLayout::kMaxIntermediatePointsCount);
  RoutePointsLayout routePoints(m_bmManager->GetUserMarksController(UserMark::Type::ROUTING));

  RouteMarkPoint * addedPoint = nullptr;
  m2::PointD addedPosition;

  for (auto const & p : routePoints.GetRoutePoints())
  {
    CHECK(p, ());
    if (p->GetRoutePointType() == RouteMarkType::Intermediate)
    {
      // Note. An added (new) intermediate point is the first intermediate point at |routePoints.GetRoutePoints()|.
      // The other intermediate points are former ones.
      if (addedPoint == nullptr)
      {
        addedPoint = p;
        addedPosition = p->GetPivot();
      }
      else
      {
        prevPoints.push_back(p);
        prevPositions.push_back(p->GetPivot());
      }
    }
  }
  if (addedPoint == nullptr)
    return;

  CheckpointPredictor predictor(m_routingSession.GetStartPoint(), m_routingSession.GetEndPoint());

  size_t const insertIndex = predictor.PredictPosition(prevPositions, addedPosition);
  addedPoint->SetIntermediateIndex(insertIndex);
  for (size_t i = 0; i < prevPoints.size(); ++i)
    prevPoints[i]->SetIntermediateIndex(i < insertIndex ? i : i + 1);

  routePoints.NotifyChanges();
}

void RoutingManager::GenerateTurnNotifications(vector<string> & turnNotifications)
{
  if (m_currentRouterType == RouterType::Taxi)
    return;

  return m_routingSession.GenerateTurnNotifications(turnNotifications);
}

void RoutingManager::BuildRoute(uint32_t timeoutSec)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ("BuildRoute"));

  auto & marksController = m_bmManager->GetUserMarksController(UserMark::Type::TRANSIT);
  marksController.Clear();
  marksController.NotifyChanges();

  auto routePoints = GetRoutePoints();
  if (routePoints.size() < 2)
  {
    CallRouteBuilded(IRouter::Cancelled, storage::TCountriesVec());
    CloseRouting(false /* remove route points */);
    return;
  }

  // Update my position.
  for (auto & p : routePoints)
  {
    if (!p.m_isMyPosition)
      continue;

    auto const & myPosition = m_bmManager->MyPositionMark();
    if (!myPosition->HasPosition())
    {
      CallRouteBuilded(IRouter::NoCurrentPosition, storage::TCountriesVec());
      return;
    }
    p.m_position = myPosition->GetPivot();
  }

  // Check for equal points.
  double const kEps = 1e-7;
  for (size_t i = 0; i < routePoints.size(); i++)
  {
    for (size_t j = i + 1; j < routePoints.size(); j++)
    {
      if (routePoints[i].m_position.EqualDxDy(routePoints[j].m_position, kEps))
      {
        CallRouteBuilded(IRouter::Cancelled, storage::TCountriesVec());
        CloseRouting(false /* remove route points */);
        return;
      }
    }
  }

  bool const isP2P = !routePoints.front().m_isMyPosition && !routePoints.back().m_isMyPosition;

  // Send tag to Push Woosh.
  {
    string tag;
    switch (m_currentRouterType)
    {
    case RouterType::Vehicle:
      tag = isP2P ? marketing::kRoutingP2PVehicleDiscovered : marketing::kRoutingVehicleDiscovered;
      break;
    case RouterType::Pedestrian:
      tag = isP2P ? marketing::kRoutingP2PPedestrianDiscovered
                  : marketing::kRoutingPedestrianDiscovered;
      break;
    case RouterType::Bicycle:
      tag = isP2P ? marketing::kRoutingP2PBicycleDiscovered : marketing::kRoutingBicycleDiscovered;
      break;
    case RouterType::Taxi:
      tag = isP2P ? marketing::kRoutingP2PTaxiDiscovered : marketing::kRoutingTaxiDiscovered;
      break;
    case RouterType::Transit:
      tag = isP2P ? marketing::kRoutingP2PTransitDiscovered : marketing::kRoutingTransitDiscovered;
      break;
    case RouterType::Count: CHECK(false, ("Bad router type", m_currentRouterType));
    }
    GetPlatform().GetMarketingService().SendPushWooshTag(tag);
  }

  if (IsRoutingActive())
    CloseRouting(false /* remove route points */);

  // Show preview.
  m2::RectD rect = ShowPreviewSegments(routePoints);
  rect.Scale(kRouteScaleMultiplier);
  m_drapeEngine.SafeCall(&df::DrapeEngine::SetModelViewRect, rect, true /* applyRotation */,
                         -1 /* zoom */, true /* isAnim */);

  m_routingSession.SetUserCurrentPosition(routePoints.front().m_position);

  vector<m2::PointD> points;
  points.reserve(routePoints.size());
  for (auto const & point : routePoints)
    points.push_back(point.m_position);

  m_routingSession.BuildRoute(Checkpoints(move(points)), timeoutSec);
}

void RoutingManager::SetUserCurrentPosition(m2::PointD const & position)
{
  if (IsRoutingActive())
    m_routingSession.SetUserCurrentPosition(position);

  if (m_routeRecommendCallback != nullptr)
  {
    // Check if we've found my position almost immediately after route points loading.
    auto constexpr kFoundLocationInterval = 2.0;
    auto const elapsed = chrono::steady_clock::now() - m_loadRoutePointsTimestamp;
    auto const sec = chrono::duration_cast<chrono::duration<double>>(elapsed).count();
    if (sec <= kFoundLocationInterval)
    {
      m_routeRecommendCallback(Recommendation::RebuildAfterPointsLoading);
      CancelRecommendation(Recommendation::RebuildAfterPointsLoading);
    }
  }
}

bool RoutingManager::DisableFollowMode()
{
  bool const disabled = m_routingSession.DisableFollowMode();
  if (disabled)
    m_drapeEngine.SafeCall(&df::DrapeEngine::DeactivateRouteFollowing);

  return disabled;
}

void RoutingManager::CheckLocationForRouting(location::GpsInfo const & info)
{
  if (!IsRoutingActive())
    return;

  auto const featureIndexGetterFn = m_callbacks.m_indexGetter;
  ASSERT(featureIndexGetterFn, ());
  RoutingSession::State const state =
      m_routingSession.OnLocationPositionChanged(info, featureIndexGetterFn());
  if (state == RoutingSession::RouteNeedRebuild)
  {
    m_routingSession.RebuildRoute(
        MercatorBounds::FromLatLon(info.m_latitude, info.m_longitude),
        [this](Route const & route, IRouter::ResultCode code) { OnRebuildRouteReady(route, code); },
        0 /* timeoutSec */, RoutingSession::State::RouteRebuilding,
        true /* adjustToPrevRoute */);
  }
}

void RoutingManager::CallRouteBuilded(IRouter::ResultCode code,
                                      storage::TCountriesVec const & absentCountries)
{
  m_routingCallback(code, absentCountries);
}

void RoutingManager::MatchLocationToRoute(location::GpsInfo & location,
                                          location::RouteMatchingInfo & routeMatchingInfo) const
{
  if (!IsRoutingActive())
    return;

  m_routingSession.MatchLocationToRoute(location, routeMatchingInfo);
}

void RoutingManager::OnLocationUpdate(location::GpsInfo & info)
{
  if (!m_drapeEngine)
    m_gpsInfoCache = my::make_unique<location::GpsInfo>(info);

  auto routeMatchingInfo = GetRouteMatchingInfo(info);
  m_drapeEngine.SafeCall(&df::DrapeEngine::SetGpsInfo, info,
                         m_routingSession.IsNavigable(), routeMatchingInfo);

  if (IsTrackingReporterEnabled())
    m_trackingReporter.AddLocation(info, m_routingSession.MatchTraffic(routeMatchingInfo));
}

location::RouteMatchingInfo RoutingManager::GetRouteMatchingInfo(location::GpsInfo & info)
{
  location::RouteMatchingInfo routeMatchingInfo;
  CheckLocationForRouting(info);
  MatchLocationToRoute(info, routeMatchingInfo);
  return routeMatchingInfo;
}

void RoutingManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine, bool is3dAllowed)
{
  m_drapeEngine.Set(engine);
  if (engine == nullptr)
    return;

  // Apply gps info which was set before drape engine creation.
  if (m_gpsInfoCache != nullptr)
  {
    auto routeMatchingInfo = GetRouteMatchingInfo(*m_gpsInfoCache);
    m_drapeEngine.SafeCall(&df::DrapeEngine::SetGpsInfo, *m_gpsInfoCache,
                           m_routingSession.IsNavigable(), routeMatchingInfo);
    m_gpsInfoCache.reset();
  }

  vector<string> symbols;
  symbols.reserve(kTransitSymbols.size() * 3);
  for (auto const & typePair : kTransitSymbols)
  {
    symbols.push_back(typePair.second + "-s");
    symbols.push_back(typePair.second + "-m");
    symbols.push_back(typePair.second + "-l");
  }
  m_drapeEngine.SafeCall(&df::DrapeEngine::RequestSymbolsSize, symbols,
                         [this](std::vector<m2::PointF> const & sizes)
                         {
                           GetPlatform().RunTask(Platform::Thread::Gui, [this, sizes]()
                           {
                             auto it = kTransitSymbols.begin();
                             for (size_t i = 0; i < sizes.size(); i += 3)
                             {
                               m_transitSymbolSizes[it->second + "-s"] = sizes[i];
                               m_transitSymbolSizes[it->second + "-m"] = sizes[i + 1];
                               m_transitSymbolSizes[it->second + "-l"] = sizes[i + 2];
                               ++it;
                             }
                           });
                         });

  // In case of the engine reinitialization recover route.
  if (IsRoutingActive())
  {
    InsertRoute(*m_routingSession.GetRoute());
    if (is3dAllowed && m_routingSession.IsFollowing())
      m_drapeEngine.SafeCall(&df::DrapeEngine::EnablePerspective);
  }
}

bool RoutingManager::HasRouteAltitude() const
{
  return m_loadAltitudes && m_routingSession.HasRouteAltitude();
}

bool RoutingManager::GenerateRouteAltitudeChart(uint32_t width, uint32_t height,
                                                vector<uint8_t> & imageRGBAData,
                                                int32_t & minRouteAltitude,
                                                int32_t & maxRouteAltitude,
                                                measurement_utils::Units & altitudeUnits) const
{
  feature::TAltitudes altitudes;
  vector<double> segDistance;

  if (!m_routingSession.GetRouteAltitudesAndDistancesM(segDistance, altitudes))
    return false;
  segDistance.insert(segDistance.begin(), 0.0);

  if (altitudes.empty())
    return false;

  if (!maps::GenerateChart(width, height, segDistance, altitudes,
                           GetStyleReader().GetCurrentStyle(), imageRGBAData))
    return false;

  auto const minMaxIt = minmax_element(altitudes.cbegin(), altitudes.cend());
  feature::TAltitude const minRouteAltitudeM = *minMaxIt.first;
  feature::TAltitude const maxRouteAltitudeM = *minMaxIt.second;

  if (!settings::Get(settings::kMeasurementUnits, altitudeUnits))
    altitudeUnits = measurement_utils::Units::Metric;

  switch (altitudeUnits)
  {
  case measurement_utils::Units::Imperial:
    minRouteAltitude = measurement_utils::MetersToFeet(minRouteAltitudeM);
    maxRouteAltitude = measurement_utils::MetersToFeet(maxRouteAltitudeM);
    break;
  case measurement_utils::Units::Metric:
    minRouteAltitude = minRouteAltitudeM;
    maxRouteAltitude = maxRouteAltitudeM;
    break;
  }
  return true;
}

bool RoutingManager::IsTrackingReporterEnabled() const
{
  if (m_currentRouterType != RouterType::Vehicle)
    return false;

  if (!m_routingSession.IsFollowing())
    return false;

  bool enableTracking = true;
  UNUSED_VALUE(settings::Get(tracking::Reporter::kEnableTrackingKey, enableTracking));
  return enableTracking;
}

void RoutingManager::SetRouter(RouterType type)
{
  ASSERT_THREAD_CHECKER(m_threadChecker, ("SetRouter"));

  if (m_currentRouterType == type)
    return;

  // Hide preview.
  HidePreviewSegments();

  SetLastUsedRouter(type);
  SetRouterImpl(type);
}

// static
uint32_t RoutingManager::InvalidRoutePointsTransactionId()
{
  return kInvalidTransactionId;
}

uint32_t RoutingManager::GenerateRoutePointsTransactionId() const
{
  static uint32_t id = kInvalidTransactionId + 1;
  return id++;
}

uint32_t RoutingManager::OpenRoutePointsTransaction()
{
  auto const id = GenerateRoutePointsTransactionId();
  m_routePointsTransactions[id].m_routeMarks = GetRoutePoints();
  return id;
}

void RoutingManager::ApplyRoutePointsTransaction(uint32_t transactionId)
{
  if (m_routePointsTransactions.find(transactionId) == m_routePointsTransactions.end())
    return;

  // If we apply a transaction we can remove all earlier transactions.
  // All older transactions must be kept since they can be applied or cancelled later.
  for (auto it = m_routePointsTransactions.begin(); it != m_routePointsTransactions.end();)
  {
    if (it->first <= transactionId)
      it = m_routePointsTransactions.erase(it);
    else
      ++it;
  }
}

void RoutingManager::CancelRoutePointsTransaction(uint32_t transactionId)
{
  auto const it = m_routePointsTransactions.find(transactionId);
  if (it == m_routePointsTransactions.end())
    return;
  auto routeMarks = it->second.m_routeMarks;

  // If we cancel a transaction we must remove all later transactions.
  for (auto it = m_routePointsTransactions.begin(); it != m_routePointsTransactions.end();)
  {
    if (it->first >= transactionId)
      it = m_routePointsTransactions.erase(it);
    else
      ++it;
  }

  // Revert route points.
  ASSERT(m_bmManager != nullptr, ());
  auto & controller = m_bmManager->GetUserMarksController(UserMark::Type::ROUTING);
  controller.Clear();
  RoutePointsLayout routePoints(controller);
  for (auto & markData : routeMarks)
    routePoints.AddRoutePoint(move(markData));
  routePoints.NotifyChanges();
}

bool RoutingManager::HasSavedRoutePoints() const
{
  auto const fileName = GetPlatform().SettingsPathForFile(kRoutePointsFile);
  return GetPlatform().IsFileExistsByFullPath(fileName);
}

bool RoutingManager::LoadRoutePoints()
{
  if (!HasSavedRoutePoints())
    return false;

  // Delete file after loading.
  auto const fileName = GetPlatform().SettingsPathForFile(kRoutePointsFile);
  MY_SCOPE_GUARD(routePointsFileGuard, bind(&FileWriter::DeleteFileX, cref(fileName)));

  string data;
  try
  {
    ReaderPtr<Reader>(GetPlatform().GetReader(fileName)).ReadAsString(data);
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Loading road points failed:", ex.Msg()));
    return false;
  }

  auto points = DeserializeRoutePoints(data);
  if (points.empty())
    return false;

  // If we have found my position, we use my position as start point.
  auto const & myPosMark = m_bmManager->MyPositionMark();
  ASSERT(m_bmManager != nullptr, ());
  m_bmManager->GetUserMarksController(UserMark::Type::ROUTING).Clear();
  for (auto & p : points)
  {
    if (p.m_pointType == RouteMarkType::Start && myPosMark->HasPosition())
    {
      RouteMarkData startPt;
      startPt.m_pointType = RouteMarkType::Start;
      startPt.m_isMyPosition = true;
      startPt.m_position = myPosMark->GetPivot();
      AddRoutePoint(move(startPt));
    }
    else
    {
      AddRoutePoint(move(p));
    }
  }

  // If we don't have my position, save loading timestamp. Probably
  // we will get my position soon.
  if (!myPosMark->HasPosition())
    m_loadRoutePointsTimestamp = chrono::steady_clock::now();

  return true;
}

void RoutingManager::SaveRoutePoints() const
{
  auto points = GetRoutePoints();
  if (points.size() < 2)
    return;

  if (points.back().m_isPassed)
    return;

  try
  {
    auto const fileName = GetPlatform().SettingsPathForFile(kRoutePointsFile);
    FileWriter writer(fileName);
    string const pointsData = SerializeRoutePoints(m_bmManager, points);
    writer.Write(pointsData.c_str(), pointsData.length());
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Saving road points failed:", ex.Msg()));
  }
}

void RoutingManager::DeleteSavedRoutePoints()
{
  if (!HasSavedRoutePoints())
    return;
  auto const fileName = GetPlatform().SettingsPathForFile(kRoutePointsFile);
  FileWriter::DeleteFileX(fileName);
}

void RoutingManager::UpdatePreviewMode()
{
  SetSubroutesVisibility(false /* visible */);
  HidePreviewSegments();
  ShowPreviewSegments(GetRoutePoints());
}

void RoutingManager::CancelPreviewMode()
{
  SetSubroutesVisibility(true /* visible */);
  HidePreviewSegments();
}

m2::RectD RoutingManager::ShowPreviewSegments(vector<RouteMarkData> const & routePoints)
{
  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (!lock)
    return MercatorBounds::FullRect();

  m2::RectD rect;
  for (size_t pointIndex = 0; pointIndex + 1 < routePoints.size(); pointIndex++)
  {
    rect.Add(routePoints[pointIndex].m_position);
    rect.Add(routePoints[pointIndex + 1].m_position);
    lock.Get()->AddRoutePreviewSegment(routePoints[pointIndex].m_position,
                                       routePoints[pointIndex + 1].m_position);
  }
  return rect;
}

void RoutingManager::HidePreviewSegments()
{
  m_drapeEngine.SafeCall(&df::DrapeEngine::RemoveAllRoutePreviewSegments);
}

void RoutingManager::CancelRecommendation(Recommendation recommendation)
{
  if (recommendation == Recommendation::RebuildAfterPointsLoading)
    m_loadRoutePointsTimestamp = chrono::steady_clock::time_point();
}

TransitRouteInfo RoutingManager::GetTransitRouteInfo() const
{
  lock_guard<mutex> lock(m_drapeSubroutesMutex);
  return m_transitRouteInfo;
}

std::vector<dp::DrapeID> RoutingManager::GetSubrouteIds() const
{
  lock_guard<mutex> lock(m_drapeSubroutesMutex);
  return m_drapeSubroutes;
}

void RoutingManager::SetSubroutesVisibility(bool visible)
{
  auto const subroutes = GetSubrouteIds();

  df::DrapeEngineLockGuard lock(m_drapeEngine);
  if (!lock)
    return;

  for (auto const & subrouteId : subroutes)
    lock.Get()->SetSubrouteVisibility(subrouteId, visible);
}
