#include "map/routing_mark.hpp"

#include "drape_frontend/color_constants.hpp"

#include <algorithm>

namespace
{
static std::string const kRouteMarkPrimaryText = "RouteMarkPrimaryText";
static std::string const kRouteMarkPrimaryTextOutline = "RouteMarkPrimaryTextOutline";
static std::string const kRouteMarkSecondaryText = "RouteMarkSecondaryText";
static std::string const kRouteMarkSecondaryTextOutline = "RouteMarkSecondaryTextOutline";

// TODO(@darina) Use separate colors.
static std::string const kTransitMarkText = "RouteMarkPrimaryText";
static std::string const kTransitMarkTextOutline = "RouteMarkPrimaryTextOutline";

float const kRouteMarkPrimaryTextSize = 11.0f;
float const kRouteMarkSecondaryTextSize = 10.0f;
float const kRouteMarkSecondaryOffsetY = 2.0f;
float const kTransitMarkTextSize = 12.0f;
}  // namespace

RouteMarkPoint::RouteMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container)
  : UserMark(ptOrg, container)
{
  m_titleDecl.m_anchor = dp::Top;
  m_titleDecl.m_primaryTextFont.m_color = df::GetColorConstant(kRouteMarkPrimaryText);
  m_titleDecl.m_primaryTextFont.m_outlineColor = df::GetColorConstant(kRouteMarkPrimaryTextOutline);
  m_titleDecl.m_primaryTextFont.m_size = kRouteMarkPrimaryTextSize;
  m_titleDecl.m_secondaryTextFont.m_color = df::GetColorConstant(kRouteMarkSecondaryText);
  m_titleDecl.m_secondaryTextFont.m_outlineColor = df::GetColorConstant(kRouteMarkSecondaryTextOutline);
  m_titleDecl.m_secondaryTextFont.m_size = kRouteMarkSecondaryTextSize;
  m_titleDecl.m_secondaryOffset = m2::PointF(0, kRouteMarkSecondaryOffsetY);

  m_markData.m_position = ptOrg;
}

dp::Anchor RouteMarkPoint::GetAnchor() const
{
  if (m_markData.m_pointType == RouteMarkType::Finish)
    return dp::Bottom;
  return dp::Center;
}

df::RenderState::DepthLayer RouteMarkPoint::GetDepthLayer() const
{
  return df::RenderState::RoutingMarkLayer;
}

void RouteMarkPoint::SetRoutePointType(RouteMarkType type)
{
  SetDirty();
  m_markData.m_pointType = type;
}

void RouteMarkPoint::SetIntermediateIndex(size_t index)
{
  SetDirty();
  m_markData.m_intermediateIndex = index;
}

void RouteMarkPoint::SetRoutePointFullType(RouteMarkType type, size_t intermediateIndex)
{
  SetRoutePointType(type);
  SetIntermediateIndex(intermediateIndex);
}

bool RouteMarkPoint::IsEqualFullType(RouteMarkType type, size_t intermediateIndex) const
{
  if (type == RouteMarkType::Intermediate)
    return m_markData.m_pointType == type && m_markData.m_intermediateIndex == intermediateIndex;
  return m_markData.m_pointType == type;
}

void RouteMarkPoint::SetIsMyPosition(bool isMyPosition)
{
  SetDirty();
  m_markData.m_isMyPosition = isMyPosition;
}

void RouteMarkPoint::SetPassed(bool isPassed)
{
  SetDirty();
  m_markData.m_isPassed = isPassed;
}

uint16_t RouteMarkPoint::GetPriority() const
{
  switch (m_markData.m_pointType)
  {
    case RouteMarkType::Start: return static_cast<uint16_t>(Priority::RouteStart);
    case RouteMarkType::Finish: return static_cast<uint16_t>(Priority::RouteFinish);
    case RouteMarkType::Intermediate:
    {
      switch (m_markData.m_intermediateIndex)
      {
        case 0: return static_cast<uint16_t>(Priority::RouteIntermediateA);
        case 1: return static_cast<uint16_t>(Priority::RouteIntermediateB);
        default: return static_cast<uint16_t>(Priority::RouteIntermediateC);
      }
    }
  }
}

uint32_t RouteMarkPoint::GetIndex() const
{
  switch (m_markData.m_pointType)
  {
    case RouteMarkType::Start: return 0;
    case RouteMarkType::Finish: return 1;
    case RouteMarkType::Intermediate: return static_cast<uint32_t >(m_markData.m_intermediateIndex + 2);
  }
}

void RouteMarkPoint::SetMarkData(RouteMarkData && data)
{
  SetDirty();
  m_markData = std::move(data);
  m_titleDecl.m_primaryText = m_markData.m_title;
  if (!m_titleDecl.m_primaryText.empty())
  {
    m_titleDecl.m_secondaryText = m_markData.m_subTitle;
    m_titleDecl.m_secondaryOptional = true;
  }
  else
    m_titleDecl.m_secondaryText.clear();
}

drape_ptr<df::UserPointMark::TitlesInfo> RouteMarkPoint::GetTitleDecl() const
{
  if (m_followingMode)
    return nullptr;

  auto titles = make_unique_dp<TitlesInfo>();
  titles->push_back(m_titleDecl);
  return titles;
}

void RouteMarkPoint::SetFollowingMode(bool enabled)
{
  if (m_followingMode == enabled)
    return;

  SetDirty();
  m_followingMode = enabled;
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> RouteMarkPoint::GetSymbolNames() const
{
  std::string name;
  switch (m_markData.m_pointType)
  {
    case RouteMarkType::Start: name = "route-point-start";  break;
    case RouteMarkType::Finish: name = "route-point-finish"; break;
    case RouteMarkType::Intermediate:
    {
      switch (m_markData.m_intermediateIndex)
      {
      case 0: name = "route-point-a"; break;
      case 1: name = "route-point-b"; break;
      case 2: name = "route-point-c"; break;
      default: name = ""; break;
      }
      break;
    }
  }
  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  symbol->insert(std::make_pair(1 /* zoomLevel */, name));
  return symbol;
}

size_t const RoutePointsLayout::kMaxIntermediatePointsCount = 3;

RoutePointsLayout::RoutePointsLayout(UserMarksController & routeMarks)
  : m_routeMarks(routeMarks)
{}

RouteMarkPoint * RoutePointsLayout::AddRoutePoint(RouteMarkData && data)
{
  if (m_routeMarks.GetUserMarkCount() == kMaxIntermediatePointsCount + 2)
    return nullptr;

  RouteMarkPoint * sameTypePoint = GetRoutePoint(data.m_pointType, data.m_intermediateIndex);
  if (sameTypePoint != nullptr)
  {
    if (data.m_pointType == RouteMarkType::Finish)
    {
      if (m_routeMarks.GetUserMarkCount() > 1)
      {
        size_t const intermediatePointsCount = m_routeMarks.GetUserMarkCount() - 2;
        sameTypePoint->SetRoutePointFullType(RouteMarkType::Intermediate, intermediatePointsCount);
      }
      else
      {
        sameTypePoint->SetRoutePointFullType(RouteMarkType::Start, 0);
      }
    }
    else
    {
      size_t const offsetIndex = data.m_pointType == RouteMarkType::Start ? 0 : data.m_intermediateIndex;

      ForEachIntermediatePoint([offsetIndex](RouteMarkPoint * mark)
      {
        if (mark->GetIntermediateIndex() >= offsetIndex)
          mark->SetIntermediateIndex(mark->GetIntermediateIndex() + 1);
      });

      if (data.m_pointType == RouteMarkType::Start)
      {
        if (m_routeMarks.GetUserMarkCount() > 1)
          sameTypePoint->SetRoutePointFullType(RouteMarkType::Intermediate, 0);
        else
          sameTypePoint->SetRoutePointFullType(RouteMarkType::Finish, 0);
      }
    }
  }
  auto userMark = m_routeMarks.CreateUserMark(data.m_position);
  ASSERT(dynamic_cast<RouteMarkPoint *>(userMark) != nullptr, ());
  RouteMarkPoint * newPoint = static_cast<RouteMarkPoint *>(userMark);
  newPoint->SetMarkData(std::move(data));

  return newPoint;
}

bool RoutePointsLayout::RemoveRoutePoint(RouteMarkType type, size_t intermediateIndex)
{
  RouteMarkPoint * point = nullptr;
  size_t index = 0;
  for (size_t sz = m_routeMarks.GetUserMarkCount(); index < sz; ++index)
  {
    RouteMarkPoint * mark = GetRouteMarkForEdit(index);
    if (mark->IsEqualFullType(type, intermediateIndex))
    {
      point = mark;
      break;
    }
  }

  if (point == nullptr)
    return false;

  if (type == RouteMarkType::Finish)
  {
    RouteMarkPoint * lastIntermediate = nullptr;
    size_t maxIntermediateIndex = 0;
    ForEachIntermediatePoint([&lastIntermediate, &maxIntermediateIndex](RouteMarkPoint * mark)
    {
      if (lastIntermediate == nullptr || mark->GetIntermediateIndex() > maxIntermediateIndex)
      {
        lastIntermediate = mark;
        maxIntermediateIndex = mark->GetIntermediateIndex();
      }
    });

    if (lastIntermediate != nullptr)
      lastIntermediate->SetRoutePointFullType(RouteMarkType::Finish, 0);
  }
  else if (type == RouteMarkType::Start)
  {
    ForEachIntermediatePoint([](RouteMarkPoint * mark)
    {
      if (mark->GetIntermediateIndex() == 0)
        mark->SetRoutePointFullType(RouteMarkType::Start, 0);
      else
        mark->SetIntermediateIndex(mark->GetIntermediateIndex() - 1);
    });
  }
  else
  {
    ForEachIntermediatePoint([point](RouteMarkPoint * mark)
    {
      if (mark->GetIntermediateIndex() > point->GetIntermediateIndex())
        mark->SetIntermediateIndex(mark->GetIntermediateIndex() - 1);
    });
  }

  m_routeMarks.DeleteUserMark(index);
  return true;
}

void RoutePointsLayout::RemoveRoutePoints()
{
  m_routeMarks.Clear();
}

void RoutePointsLayout::RemoveIntermediateRoutePoints()
{
  for (size_t i = 0; i < m_routeMarks.GetUserMarkCount();)
  {
    RouteMarkPoint const * mark = GetRouteMark(i);
    if (mark->GetRoutePointType() == RouteMarkType::Intermediate)
      m_routeMarks.DeleteUserMark(i);
    else
      ++i;
  }
}

bool RoutePointsLayout::MoveRoutePoint(RouteMarkType currentType, size_t currentIntermediateIndex,
                                       RouteMarkType destType, size_t destIntermediateIndex)
{
  RouteMarkPoint * point = GetRoutePoint(currentType, currentIntermediateIndex);
  if (point == nullptr)
    return false;

  RouteMarkData data = point->GetMarkData();
  data.m_pointType = destType;
  data.m_intermediateIndex = destIntermediateIndex;

  RemoveRoutePoint(currentType, currentIntermediateIndex);

  AddRoutePoint(std::move(data));
  return true;
}

void RoutePointsLayout::PassRoutePoint(RouteMarkType type, size_t intermediateIndex)
{
  RouteMarkPoint * point = GetRoutePoint(type, intermediateIndex);
  if (point == nullptr)
    return;
  point->SetPassed(true);
  point->SetIsVisible(false);
}

void RoutePointsLayout::SetFollowingMode(bool enabled)
{
  for (size_t i = 0, sz = m_routeMarks.GetUserMarkCount(); i < sz; ++i)
    GetRouteMarkForEdit(i)->SetFollowingMode(enabled);
}

RouteMarkPoint * RoutePointsLayout::GetRoutePoint(RouteMarkType type, size_t intermediateIndex)
{
  for (size_t i = 0, sz = m_routeMarks.GetUserMarkCount(); i < sz; ++i)
  {
    RouteMarkPoint * mark = GetRouteMarkForEdit(i);
    if (mark->IsEqualFullType(type, intermediateIndex))
      return mark;
  }
  return nullptr;
}

RouteMarkPoint * RoutePointsLayout::GetMyPositionPoint()
{
  for (size_t i = 0, sz = m_routeMarks.GetUserMarkCount(); i < sz; ++i)
  {
    RouteMarkPoint * mark = GetRouteMarkForEdit(i);
    if (mark->IsMyPosition())
      return mark;
  }
  return nullptr;
}

std::vector<RouteMarkPoint *> RoutePointsLayout::GetRoutePoints()
{
  std::vector<RouteMarkPoint *> points;
  points.reserve(m_routeMarks.GetUserMarkCount());
  RouteMarkPoint * startPoint = nullptr;
  RouteMarkPoint * finishPoint = nullptr;
  for (size_t i = 0, sz = m_routeMarks.GetUserMarkCount(); i < sz; ++i)
  {
    RouteMarkPoint * p = GetRouteMarkForEdit(i);
    if (p->GetRoutePointType() == RouteMarkType::Start)
      startPoint = p;
    else if (p->GetRoutePointType() == RouteMarkType::Finish)
      finishPoint = p;
    else
      points.push_back(p);
  }
  std::sort(points.begin(), points.end(), [](RouteMarkPoint const * p1, RouteMarkPoint const * p2)
  {
    return p1->GetIntermediateIndex() < p2->GetIntermediateIndex();
  });
  if (startPoint != nullptr)
    points.insert(points.begin(), startPoint);
  if (finishPoint != nullptr)
    points.push_back(finishPoint);
  return points;
}

size_t RoutePointsLayout::GetRoutePointsCount() const
{
  return m_routeMarks.GetUserMarkCount();
}

RouteMarkPoint * RoutePointsLayout::GetRouteMarkForEdit(size_t index)
{
  auto userMark = m_routeMarks.GetUserMarkForEdit(index);
  ASSERT(dynamic_cast<RouteMarkPoint *>(userMark) != nullptr, ());
  return static_cast<RouteMarkPoint *>(userMark);
}

RouteMarkPoint const * RoutePointsLayout::GetRouteMark(size_t index)
{
  auto userMark = m_routeMarks.GetUserMark(index);
  ASSERT(dynamic_cast<RouteMarkPoint const *>(userMark) != nullptr, ());
  return static_cast<RouteMarkPoint const *>(userMark);
}

void RoutePointsLayout::ForEachIntermediatePoint(TRoutePointCallback const & fn)
{
  for (size_t i = 0, sz = m_routeMarks.GetUserMarkCount(); i < sz; ++i)
  {
    RouteMarkPoint * mark = GetRouteMarkForEdit(i);
    if (mark->GetRoutePointType() == RouteMarkType::Intermediate)
      fn(mark);
  }
}

void RoutePointsLayout::NotifyChanges()
{
  m_routeMarks.NotifyChanges();
}

TransitMark::TransitMark(m2::PointD const & ptOrg, UserMarkContainer * container)
    : UserMark(ptOrg, container)
{}

void TransitMark::SetFeatureId(FeatureID featureId)
{
  SetDirty();
  m_featureId = featureId;
}

void TransitMark::SetIndex(uint32_t index)
{
  SetDirty();
  m_index = index;
}

void TransitMark::SetPriority(Priority priority)
{
  SetDirty();
  m_priority = priority;
}

void TransitMark::SetMinZoom(int minZoom)
{
  SetDirty();
  m_minZoom = minZoom;
}

void TransitMark::SetMinTitleZoom(int minTitleZoom)
{
  SetDirty();
  m_minTitleZoom = minTitleZoom;
}

void TransitMark::AddTitle(dp::TitleDecl const & titleDecl)
{
  SetDirty();
  m_titles.push_back(titleDecl);
}

drape_ptr<df::UserPointMark::TitlesInfo> TransitMark::GetTitleDecl() const
{
  auto titles = make_unique_dp<TitlesInfo>(m_titles);
  return titles;
}

void TransitMark::SetSymbolNames(std::map<int, std::string> const & symbolNames)
{
  SetDirty();
  m_symbolNames = symbolNames;
}

void TransitMark::SetColoredSymbols(std::map<int, df::ColoredSymbolViewParams> const & symbolParams)
{
  SetDirty();
  m_coloredSymbols = symbolParams;
}

drape_ptr<df::UserPointMark::ColoredSymbolZoomInfo> TransitMark::GetColoredSymbols() const
{
  if (m_coloredSymbols.empty())
    return nullptr;
  return make_unique_dp<ColoredSymbolZoomInfo>(m_coloredSymbols);
}

void TransitMark::SetSymbolSizes(SymbolSizes const & symbolSizes)
{
  m_symbolSizes = symbolSizes;
}

drape_ptr<df::UserPointMark::SymbolSizes> TransitMark::GetSymbolSizes() const
{
  if (m_symbolSizes.empty())
    return nullptr;
  return make_unique_dp<SymbolSizes>(m_symbolSizes);
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> TransitMark::GetSymbolNames() const
{
  if (m_symbolNames.empty())
    return nullptr;
  return make_unique_dp<SymbolNameZoomInfo>(m_symbolNames);
};

// static
void TransitMark::GetDefaultTransitTitle(dp::TitleDecl & titleDecl)
{
  titleDecl = dp::TitleDecl();
  titleDecl.m_primaryTextFont.m_color = df::GetColorConstant(kTransitMarkText);
  titleDecl.m_primaryTextFont.m_outlineColor = df::GetColorConstant(kTransitMarkTextOutline);
  titleDecl.m_primaryTextFont.m_size = kTransitMarkTextSize;
  titleDecl.m_secondaryTextFont.m_color = df::GetColorConstant(kTransitMarkText);
  titleDecl.m_secondaryTextFont.m_outlineColor = df::GetColorConstant(kTransitMarkTextOutline);
  titleDecl.m_secondaryTextFont.m_size = kTransitMarkTextSize;
}
