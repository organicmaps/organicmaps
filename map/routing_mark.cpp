#include "map/routing_mark.hpp"

#include <algorithm>

static int8_t const kMaxIntermediatePointsCount = 3;

RouteMarkPoint::RouteMarkPoint(const m2::PointD & ptOrg, UserMarkContainer * container)
  : UserMark(ptOrg, container)
{}

bool RouteMarkPoint::IsVisible() const
{
  return m_isVisible;
}

void RouteMarkPoint::SetIsVisible(bool isVisible)
{
  m_isVisible = isVisible;
}

std::string RouteMarkPoint::GetSymbolName() const
{
  switch (m_pointType)
  {
  case RouteMarkType::Start:
    return "placemark-blue";
  case RouteMarkType::Intermediate:
    if (m_intermediateIndex == 0)
      return "placemark-yellow";
    if (m_intermediateIndex == 1)
      return "placemark-orange";
    return "placemark-red";
  case RouteMarkType::Finish:
    return "placemark-green";
  }
}

RouteUserMarkContainer::RouteUserMarkContainer(double layerDepth, Framework & fm)
  : UserMarkContainer(layerDepth, UserMarkType::ROUTING_MARK, fm)
{
}

UserMark * RouteUserMarkContainer::AllocateUserMark(m2::PointD const & ptOrg)
{
  return new RouteMarkPoint(ptOrg, this);
}

RoutePointsLayout::RoutePointsLayout(UserMarksController & routeMarks)
  : m_routeMarks(routeMarks)
{}

RouteMarkPoint * RoutePointsLayout::AddRoutePoint(m2::PointD const & ptOrg, RouteMarkType type, int8_t intermediateIndex)
{
  if (m_routeMarks.GetUserMarkCount() == kMaxIntermediatePointsCount + 2)
    return nullptr;

  RouteMarkPoint * sameTypePoint = GetRoutePoint(type, intermediateIndex);
  if (sameTypePoint != nullptr)
  {
    if (type == RouteMarkType::Finish)
    {
      int8_t const intermediatePointsCount = std::max((int)m_routeMarks.GetUserMarkCount() - 2, 0);
      sameTypePoint->SetRoutePointType(RouteMarkType::Intermediate);
      sameTypePoint->SetIntermediateIndex(intermediatePointsCount);
    }
    else
    {
      int8_t const offsetIndex = type == RouteMarkType::Start ? 0 : intermediateIndex;

      ForEachIntermediatePoint([offsetIndex](RouteMarkPoint * mark)
      {
        if (mark->GetIntermediateIndex() >= offsetIndex)
          mark->SetIntermediateIndex(mark->GetIntermediateIndex() + 1);
      });

      if (type == RouteMarkType::Start)
      {
        sameTypePoint->SetRoutePointType(RouteMarkType::Intermediate);
        sameTypePoint->SetIntermediateIndex(0);
      }
    }
  }
  RouteMarkPoint * newPoint = static_cast<RouteMarkPoint*>(m_routeMarks.CreateUserMark(ptOrg));
  newPoint->SetRoutePointType(type);
  newPoint->SetIntermediateIndex(intermediateIndex);

  return newPoint;
}


bool RoutePointsLayout::RemoveRoutePoint(RouteMarkType type, int8_t intermediateIndex)
{
  RouteMarkPoint * point = nullptr;
  size_t index = 0;
  for (size_t sz = m_routeMarks.GetUserMarkCount(); index < sz; ++index)
  {
    RouteMarkPoint * mark = static_cast<RouteMarkPoint*>(m_routeMarks.GetUserMarkForEdit(index));
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

bool RoutePointsLayout::MoveRoutePoint(RouteMarkType currentType, int8_t currentIntermediateIndex,
                                              RouteMarkType destType, int8_t destIntermediateIndex)
{
  RouteMarkPoint * point = GetRoutePoint(currentType, currentIntermediateIndex);
  if (point == nullptr)
    return false;

  m2::PointD const pt = point->GetPivot();
  bool const isVisible = point->IsVisible();

  RemoveRoutePoint(currentType, currentIntermediateIndex);
  RouteMarkPoint * point2 = AddRoutePoint(pt, destType, destIntermediateIndex);
  point2->SetIsVisible(isVisible);
  return true;
}

RouteMarkPoint * RoutePointsLayout::GetRoutePoint(RouteMarkType type, int8_t intermediateIndex)
{
  for (size_t i = 0, sz = m_routeMarks.GetUserMarkCount(); i < sz; ++i)
  {
    RouteMarkPoint * mark = static_cast<RouteMarkPoint*>(m_routeMarks.GetUserMarkForEdit(i));
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

void RoutePointsLayout::ForEachIntermediatePoint(TRoutePointCallback const & fn)
{
  for (size_t i = 0, sz = m_routeMarks.GetUserMarkCount(); i < sz; ++i)
  {
    RouteMarkPoint * mark = static_cast<RouteMarkPoint*>(m_routeMarks.GetUserMarkForEdit(i));
    if (mark->GetRoutePointType() == RouteMarkType::Intermediate)
      fn(mark);
  }
}
