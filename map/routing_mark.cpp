#include "map/routing_mark.hpp"

#include <algorithm>

RouteMarkPoint::RouteMarkPoint(m2::PointD const & ptOrg,
                               UserMarkContainer * container)
  : UserMark(ptOrg, container)
{
  m_titleDecl.m_anchor = dp::Top;
  m_titleDecl.m_primaryTextFont.m_color = dp::Color::Black();
  m_titleDecl.m_primaryTextFont.m_outlineColor = dp::Color::White();
  m_titleDecl.m_primaryTextFont.m_size = 22;

  m_markData.m_position = ptOrg;
}

dp::Anchor RouteMarkPoint::GetAnchor() const
{
  if (m_markData.m_pointType == RouteMarkType::Finish)
    return dp::Bottom;
  return dp::Center;
}

dp::GLState::DepthLayer RouteMarkPoint::GetDepthLayer() const
{
  return dp::GLState::RoutingMarkLayer;
}

void RouteMarkPoint::SetRoutePointType(RouteMarkType type)
{
  SetDirty();
  m_markData.m_pointType = type;
}

void RouteMarkPoint::SetIntermediateIndex(int8_t index)
{
  SetDirty();
  m_markData.m_intermediateIndex = index;
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

void RouteMarkPoint::SetMarkData(RouteMarkData && data)
{
  SetDirty();
  m_markData = std::move(data);
  m_titleDecl.m_primaryText = m_markData.m_name;
}

drape_ptr<dp::TitleDecl> RouteMarkPoint::GetTitleDecl() const
{
  drape_ptr<dp::TitleDecl> titleDecl = make_unique_dp<dp::TitleDecl>(m_titleDecl);
  return titleDecl;
}

std::string RouteMarkPoint::GetSymbolName() const
{
  switch (m_markData.m_pointType)
  {
  case RouteMarkType::Start: return "route-point-start";
  case RouteMarkType::Intermediate:
  {
    switch (m_markData.m_intermediateIndex)
    {
    case 0: return "route-point-a";
    case 1: return "route-point-b";
    case 2: return "route-point-c";
    default: return "";
    }
  }
  case RouteMarkType::Finish: return "route-point-finish";
  }
}

RouteUserMarkContainer::RouteUserMarkContainer(double layerDepth, Framework & fm)
  : UserMarkContainer(layerDepth, UserMarkType::ROUTING_MARK, fm)
{}

UserMark * RouteUserMarkContainer::AllocateUserMark(m2::PointD const & ptOrg)
{
  return new RouteMarkPoint(ptOrg, this);
}

int8_t const RoutePointsLayout::kMaxIntermediatePointsCount = 1;

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
      int const intermediatePointsCount = std::max(static_cast<int>(m_routeMarks.GetUserMarkCount()) - 2, 0);
      sameTypePoint->SetRoutePointType(RouteMarkType::Intermediate);
      sameTypePoint->SetIntermediateIndex(static_cast<int8_t>(intermediatePointsCount));
    }
    else
    {
      int const offsetIndex = data.m_pointType == RouteMarkType::Start ? 0 : data.m_intermediateIndex;

      ForEachIntermediatePoint([offsetIndex](RouteMarkPoint * mark)
      {
        if (mark->GetIntermediateIndex() >= offsetIndex)
          mark->SetIntermediateIndex(static_cast<int8_t>(mark->GetIntermediateIndex() + 1));
      });

      if (data.m_pointType == RouteMarkType::Start)
      {
        sameTypePoint->SetRoutePointType(RouteMarkType::Intermediate);
        sameTypePoint->SetIntermediateIndex(0);
      }
    }
  }
  auto userMark = m_routeMarks.CreateUserMark(data.m_position);
  ASSERT(dynamic_cast<RouteMarkPoint *>(userMark) != nullptr, ());
  RouteMarkPoint * newPoint = static_cast<RouteMarkPoint *>(userMark);
  newPoint->SetMarkData(std::move(data));

  return newPoint;
}

bool RoutePointsLayout::RemoveRoutePoint(RouteMarkType type, int8_t intermediateIndex)
{
  RouteMarkPoint * point = nullptr;
  size_t index = 0;
  for (size_t sz = m_routeMarks.GetUserMarkCount(); index < sz; ++index)
  {
    auto userMark = m_routeMarks.GetUserMarkForEdit(index);
    ASSERT(dynamic_cast<RouteMarkPoint *>(userMark) != nullptr, ());
    RouteMarkPoint * mark = static_cast<RouteMarkPoint *>(userMark);
    if (mark->GetRoutePointType() == type && mark->GetIntermediateIndex() == intermediateIndex)
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
    int8_t maxIntermediateIndex = -1;
    ForEachIntermediatePoint([&lastIntermediate, &maxIntermediateIndex](RouteMarkPoint * mark)
    {
      if (mark->GetIntermediateIndex() > maxIntermediateIndex)
      {
        lastIntermediate = mark;
        maxIntermediateIndex = mark->GetIntermediateIndex();
      }
    });

    if (lastIntermediate != nullptr)
      lastIntermediate->SetRoutePointType(RouteMarkType::Finish);
  }
  else if (type == RouteMarkType::Start)
  {
    ForEachIntermediatePoint([](RouteMarkPoint * mark)
    {
      if (mark->GetIntermediateIndex() == 0)
        mark->SetRoutePointType(RouteMarkType::Start);
      else
        mark->SetIntermediateIndex(static_cast<int8_t>(mark->GetIntermediateIndex() - 1));
    });
  }
  else
  {
    ForEachIntermediatePoint([point](RouteMarkPoint * mark)
    {
      if (mark->GetIntermediateIndex() > point->GetIntermediateIndex())
        mark->SetIntermediateIndex(static_cast<int8_t>(mark->GetIntermediateIndex() - 1));
    });
  }

  m_routeMarks.DeleteUserMark(index);
  return true;
}

void RoutePointsLayout::RemoveIntermediateRoutePoints()
{
  for (size_t i = 0; i < m_routeMarks.GetUserMarkCount();)
  {
    auto userMark = m_routeMarks.GetUserMark(i);
    ASSERT(dynamic_cast<RouteMarkPoint const *>(userMark) != nullptr, ());
    RouteMarkPoint const * mark = static_cast<RouteMarkPoint const *>(userMark);
    if (mark->GetRoutePointType() == RouteMarkType::Intermediate)
      m_routeMarks.DeleteUserMark(i);
    else
      ++i;
  }
}

bool RoutePointsLayout::MoveRoutePoint(RouteMarkType currentType, int8_t currentIntermediateIndex,
                                       RouteMarkType destType, int8_t destIntermediateIndex)
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

void RoutePointsLayout::PassRoutePoint(RouteMarkType type, int8_t intermediateIndex)
{
  RouteMarkPoint * point = GetRoutePoint(type, intermediateIndex);
  if (point == nullptr)
    return;
  point->SetPassed(true);
  point->SetIsVisible(false);
}

RouteMarkPoint * RoutePointsLayout::GetRoutePoint(RouteMarkType type, int8_t intermediateIndex)
{
  for (size_t i = 0, sz = m_routeMarks.GetUserMarkCount(); i < sz; ++i)
  {
    auto userMark = m_routeMarks.GetUserMarkForEdit(i);
    ASSERT(dynamic_cast<RouteMarkPoint *>(userMark) != nullptr, ());
    RouteMarkPoint * mark = static_cast<RouteMarkPoint *>(userMark);
    if (mark->GetRoutePointType() != type)
      continue;

    if (type == RouteMarkType::Intermediate)
    {
      if (mark->GetIntermediateIndex() == intermediateIndex)
        return mark;
    }
    else
    {
      return mark;
    }
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
    auto userMark = m_routeMarks.GetUserMarkForEdit(i);
    ASSERT(dynamic_cast<RouteMarkPoint *>(userMark) != nullptr, ());
    RouteMarkPoint * p = static_cast<RouteMarkPoint *>(userMark);
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

void RoutePointsLayout::ForEachIntermediatePoint(TRoutePointCallback const & fn)
{
  for (size_t i = 0, sz = m_routeMarks.GetUserMarkCount(); i < sz; ++i)
  {
    auto userMark = m_routeMarks.GetUserMarkForEdit(i);
    ASSERT(dynamic_cast<RouteMarkPoint *>(userMark) != nullptr, ());
    RouteMarkPoint * mark = static_cast<RouteMarkPoint *>(userMark);
    if (mark->GetRoutePointType() == RouteMarkType::Intermediate)
      fn(mark);
  }
}
