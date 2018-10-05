#include "transit_display.hpp"

#include "drape_frontend/route_renderer.hpp"
#include "drape_frontend/visual_params.hpp"

#include "routing/routing_session.hpp"

#include <memory>

using namespace std;
using namespace routing;

map<TransitType, string> const kTransitSymbols = {
    {TransitType::Subway, "transit_subway"},
    {TransitType::LightRail, "transit_light_rail"},
    {TransitType::Monorail, "transit_monorail"},
    {TransitType::Train, "transit_train"}
};

namespace
{
float const kStopMarkerScale = 2.2f;
float const kTransferMarkerScale = 4.0f;
const float kGateBgScale = 1.2f;

const int kSmallIconZoom = 1;
const int kMediumIconZoom = 10;

const int kMinStopTitleZoom = 13;

const int kTransferTitleOffset = 2;
const int kStopTitleOffset = 0;
const int kGateTitleOffset = 0;

TransitType GetTransitType(string const &type)
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

uint32_t ColorToARGB(df::ColorConstant const & colorConstant)
{
  auto const color = df::GetColorConstant(colorConstant);
  return color.GetAlpha() << 24 | color.GetRed() << 16 | color.GetGreen() << 8 | color.GetBlue();
}

vector<m2::PointF> GetTransitMarkerSizes(float markerScale, float maxRouteWidth)
{
  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());
  vector<m2::PointF> markerSizes;
  markerSizes.reserve(df::kRouteHalfWidthInPixelTransit.size());
  for (auto const halfWidth : df::kRouteHalfWidthInPixelTransit)
  {
    float const d = 2.0f * std::min(halfWidth * vs, maxRouteWidth * 0.5f) * markerScale;
    markerSizes.push_back(m2::PointF(d, d));
  }
  return markerSizes;
}
}  // namespace

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

void AddTransitGateSegment(m2::PointD const & destPoint, df::ColorConstant const & color, df::Subroute & subroute)
{
  ASSERT_GREATER(subroute.m_polyline.GetSize(), 0, ());
  df::SubrouteStyle style(color, df::RoutePattern(4.0, 2.0));
  style.m_startIndex = subroute.m_polyline.GetSize() - 1;
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
  return m_routeInfo;
}

void TransitRouteDisplay::ProcessSubroute(vector<RouteSegment> const & segments, df::Subroute & subroute)
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
    return;

  subroute.m_maxPixelWidth = m_maxSubrouteWidth;
  subroute.m_styleType = df::SubrouteStyleType::Multiple;
  subroute.m_style.clear();
  subroute.m_style.reserve(segments.size());

  df::ColorConstant lastColor;
  m2::PointD lastDir;
  auto lastLineId = transit::kInvalidLineId;

  df::SubrouteMarker marker;
  TransitMarkInfo transitMarkInfo;
  TransitType transitType = TransitType::Pedestrian;

  double prevDistance = m_routeInfo.m_totalDistInMeters;
  double prevTime = m_routeInfo.m_totalTimeInSec;

  bool pendingEntrance = false;

  for (auto const & s : segments)
  {
    auto const time = static_cast<int>(ceil(s.GetTimeFromBeginningSec() - prevTime));
    auto const distance = s.GetDistFromBeginningMeters() - prevDistance;
    prevDistance = s.GetDistFromBeginningMeters();
    prevTime = s.GetTimeFromBeginningSec();

    if (!s.HasTransitInfo())
    {
      m_routeInfo.AddStep(TransitStepInfo(TransitType::Pedestrian, distance, time));

      AddTransitPedestrianSegment(s.GetJunction().GetPoint(), subroute);
      lastColor = "";
      lastLineId = transit::kInvalidLineId;
      transitType = TransitType::Pedestrian;
      continue;
    }

    auto const mwmId = m_getMwmIdFn(s.GetSegment().GetMwmId());

    auto const & transitInfo = s.GetTransitInfo();
    auto const & displayInfo = *transitDisplayInfos.at(mwmId).get();

    if (transitInfo.GetType() == TransitInfo::Type::Edge)
    {
      auto const & edge = transitInfo.GetEdge();

      auto const currentLineId = edge.m_lineId;

      auto const & line = displayInfo.m_lines.at(currentLineId);
      auto const currentColor = df::GetTransitColorName(line.GetColor());
      transitType = GetTransitType(line.GetType());

      m_routeInfo.AddStep(TransitStepInfo(transitType, distance, time,
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

      if (id1 != id2)
      {
        bool const isInverted = id1 > id2;
        AddTransitShapes(edge.m_shapeIds, displayInfo.m_shapes, currentColor, isInverted, subroute);
      }

      ASSERT_GREATER(subroute.m_polyline.GetSize(), 1, ());
      auto const & p1 = *(subroute.m_polyline.End() - 2);
      auto const & p2 = *(subroute.m_polyline.End() - 1);
      m2::PointD currentDir = (p2 - p1).Normalize();

      if (lastLineId != currentLineId)
      {
        if (lastLineId != transit::kInvalidLineId)
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
      lastLineId = currentLineId;

      if (marker.m_colors.size() > 1)
      {
        marker.m_innerColor = df::kTransitStopInnerMarkerColor;
        marker.m_up = (currentDir - lastDir).Normalize();
        if (m2::CrossProduct(marker.m_up, -lastDir) < 0)
          marker.m_up = -marker.m_up;
      }

      subroute.m_markers.push_back(marker);
      marker = df::SubrouteMarker();

      m_transitMarks.push_back(transitMarkInfo);
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
      if (lastLineId != transit::kInvalidLineId)
      {
        m_routeInfo.AddStep(TransitStepInfo(TransitType::Pedestrian, distance, time));

        AddTransitGateSegment(s.GetJunction().GetPoint(), lastColor, subroute);

        subroute.m_markers.push_back(marker);
        marker = df::SubrouteMarker();

        transitMarkInfo.m_type = TransitMarkInfo::Type::KeyStop;
        transitMarkInfo.m_symbolName = kTransitSymbols.at(transitType);
        transitMarkInfo.m_color = lastColor;
        m_transitMarks.push_back(transitMarkInfo);
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
          symbolName = symbolName.substr(0, symbolName.rfind('-'));
        }

        gateMarkInfo.m_featureId = fid;
        gateMarkInfo.m_symbolName = symbolName;
        auto const title = m_getStringsBundleFn().GetString(pendingEntrance ? "core_entrance" : "core_exit");
        gateMarkInfo.m_titles.push_back(TransitTitle(title, df::GetTransitTextColorName("default")));
      }

      m_transitMarks.push_back(gateMarkInfo);
    }
  }

  m_routeInfo.m_totalDistInMeters = prevDistance;
  m_routeInfo.m_totalTimeInSec = static_cast<int>(ceil(prevTime));
}

void TransitRouteDisplay::CollectTransitDisplayInfo(vector<RouteSegment> const & segments,
                                                    TransitDisplayInfos & transitDisplayInfos)
{
  for (auto const & s : segments)
  {
    if (!s.HasTransitInfo())
      continue;

    auto const mwmId = m_getMwmIdFn(s.GetSegment().GetMwmId());

    auto & mwmTransit = transitDisplayInfos[mwmId];
    if (mwmTransit == nullptr)
      mwmTransit = make_unique<TransitDisplayInfo>();

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
      df::UserPointMark::SymbolNameZoomInfo symbolNames;
      symbolNames[kSmallIconZoom] = mark.m_symbolName + "-s";
      symbolNames[kMediumIconZoom] = mark.m_symbolName + "-m";
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
        params.m_radiusInPixels = max(sz.x, sz.y) * 0.5f;
        params.m_color = dp::Color::Transparent();
        if (coloredSymbol.m_zoomInfo.empty() ||
            coloredSymbol.m_zoomInfo.rbegin()->second.m_radiusInPixels != params.m_radiusInPixels)
        {
          coloredSymbol.m_zoomInfo.insert(make_pair(zoomLevel, params));
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
        params.m_radiusInPixels = max(sz.x, sz.y) * kGateBgScale * 0.5f;
        coloredSymbol.m_zoomInfo[kSmallIconZoom] = params;

        sz = m_symbolSizes.at(symbolNames[kMediumIconZoom]);
        params.m_radiusInPixels = max(sz.x, sz.y) * kGateBgScale * 0.5f;
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
          params.m_radiusInPixels = max(sz.x, sz.y) * 0.5f;
          params.m_color = dp::Color::Transparent();
          if (coloredSymbol.m_zoomInfo.empty() ||
              coloredSymbol.m_zoomInfo.rbegin()->second.m_radiusInPixels != params.m_radiusInPixels)
          {
            coloredSymbol.m_zoomInfo.insert(make_pair(zoomLevel, params));
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
