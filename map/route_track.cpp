#include "map/route_track.hpp"

#include "map/location_state.hpp"

#include "indexer/scales.hpp"

#include "std/array.hpp"

namespace
{
  pair<m2::PointD, m2::PointD> ShiftArrow(pair<m2::PointD, m2::PointD> const & arrowDirection)
  {
    return pair<m2::PointD, m2::PointD>(arrowDirection.first - (arrowDirection.second - arrowDirection.first),
                                        arrowDirection.first);
  }

//  void DrawArrowTriangle(graphics::Screen * dlScreen, pair<m2::PointD, m2::PointD> const & arrowDirection,
//                         double arrowWidth, double arrowLength, graphics::Color arrowColor, double arrowDepth)
//  {
//    ASSERT(dlScreen, ());

//    array<m2::PointF, 3> arrow;
//    m2::GetArrowPoints(arrowDirection.first, arrowDirection.second, arrowWidth, arrowLength, arrow);
//    dlScreen->drawConvexPolygon(&arrow[0], arrow.size(), arrowColor, arrowDepth);
//  }
}

bool ClipArrowBodyAndGetArrowDirection(vector<m2::PointD> & ptsTurn, pair<m2::PointD, m2::PointD> & arrowDirection,
                                       size_t turnIndex, double beforeTurn, double afterTurn, double arrowLength)
{
  size_t const ptsTurnSz = ptsTurn.size();
  ASSERT_LESS(turnIndex, ptsTurnSz, ());
  ASSERT_GREATER(ptsTurnSz, 1, ());

  /// Clipping after turnIndex
  size_t i = turnIndex;
  double len = 0, vLen = 0;
  while (len < afterTurn)
  {
    if (i > ptsTurnSz - 2)
      return false;
    vLen = ptsTurn[i + 1].Length(ptsTurn[i]);
    len += vLen;
    i += 1;
  }
  if (my::AlmostEqualULPs(vLen, 0.))
    return false;

  double lenForArrow = len - afterTurn;
  double vLenForArrow = lenForArrow;
  size_t j = i;
  while (lenForArrow < arrowLength)
  {
    if (j > ptsTurnSz - 2)
      return false;
    vLenForArrow = ptsTurn[j + 1].Length(ptsTurn[j]);
    lenForArrow +=  vLenForArrow;
    j += 1;
  }
  ASSERT_GREATER(j, 0, ());
  if (m2::AlmostEqualULPs(ptsTurn[j - 1], ptsTurn[j]))
    return false;
  m2::PointD arrowEnd = m2::PointAtSegment(ptsTurn[j - 1], ptsTurn[j], vLenForArrow - (lenForArrow - arrowLength));

  if (my::AlmostEqualULPs(len, afterTurn))
    ptsTurn.resize(i + 1);
  else
  {
    if (!m2::AlmostEqualULPs(ptsTurn[i], ptsTurn[i - 1]))
    {
      m2::PointD const p = m2::PointAtSegment(ptsTurn[i - 1], ptsTurn[i], vLen - (len - afterTurn));
      ptsTurn[i] = p;
      ptsTurn.resize(i + 1);
    }
    else
      ptsTurn.resize(i);
  }

  // Calculating arrow direction
  arrowDirection.first = ptsTurn.back();
  arrowDirection.second = arrowEnd;
  arrowDirection = ShiftArrow(arrowDirection);

  /// Clipping before turnIndex
  i = turnIndex;
  len = 0;
  while (len < beforeTurn)
  {
    if (i < 1)
      return false;
    vLen = ptsTurn[i - 1].Length(ptsTurn[i]);
    len += vLen;
    i -= 1;
  }
  if (my::AlmostEqualULPs(vLen, 0.))
    return false;

  if (my::AlmostEqualULPs(len, beforeTurn))
  {
    if (i != 0)
    {
      ptsTurn.erase(ptsTurn.begin(), ptsTurn.begin() + i);
      return true;
    }
  }
  else
  {
    if (!m2::AlmostEqualULPs(ptsTurn[i], ptsTurn[i + 1]))
    {
      m2::PointD const p = m2::PointAtSegment(ptsTurn[i + 1], ptsTurn[i], vLen - (len - beforeTurn));
      ptsTurn[i] = p;
      if (i != 0)
      {
        ptsTurn.erase(ptsTurn.begin(), ptsTurn.begin() + i);
        return true;
      }
    }
    else
    {
      ptsTurn.erase(ptsTurn.begin(), ptsTurn.begin() + i);
      return true;
    }
  }
  return true;
}

bool MergeArrows(vector<m2::PointD> & ptsCurrentTurn, vector<m2::PointD> const & ptsNextTurn, double bodyLen, double arrowLen)
{
  ASSERT_GREATER_OR_EQUAL(ptsCurrentTurn.size(), 2, ());
  ASSERT_GREATER_OR_EQUAL(ptsNextTurn.size(), 2, ());
  ASSERT_GREATER(bodyLen, 0, ());
  ASSERT_GREATER(arrowLen, 0, ());
  ASSERT_GREATER(bodyLen, arrowLen, ());

  double const distBetweenBodies = ptsCurrentTurn.back().Length(ptsNextTurn.front());
  /// The most likely the function returns here because the arrows are usually far to each other
  if (distBetweenBodies > bodyLen)
    return false;

  /// The arrows are close to each other or intersected
  double const distBetweenBodies2 = ptsCurrentTurn[ptsCurrentTurn.size() - 2].Length(ptsNextTurn[1]);
  if (distBetweenBodies < distBetweenBodies2)
  {
    /// The arrows bodies are not intersected
    if (distBetweenBodies > arrowLen)
      return false;
    else
    {
      ptsCurrentTurn.push_back(ptsNextTurn.front());
      return true;
    }
  }
  else
  {
    /// The arrows bodies are intersected. Make ptsCurrentTurn shorter if possible to prevent double rendering.
    /// ptsNextTurn[0] is not a point of route network graph. It is generated while clipping.
    /// The first point of route network graph is ptsNextTurn[1]. The same with the end of ptsCurrentTurn
    m2::PointD const pivotPnt = ptsNextTurn[1]; 
    auto const currentTurnBeforeEnd = ptsCurrentTurn.end() - 1;
    for (auto t = ptsCurrentTurn.begin() + 1; t != currentTurnBeforeEnd; ++t)
    {
      if (m2::AlmostEqualULPs(*t, pivotPnt))
      {
        ptsCurrentTurn.erase(t + 1, ptsCurrentTurn.end());
        return true;
      }
    }
    return true;
  }
}

/*
void RouteTrack::CreateDisplayListArrows(graphics::Screen * dlScreen, MatrixT const & matrix, double visualScale) const
{
  double const beforeTurn = 13. * visualScale;
  double const afterTurn = 13. * visualScale;
  double const arrowWidth = 10. * visualScale;
  double const arrowLength = 19. * visualScale;
  double const arrowBodyWidth = 8. * visualScale;
  double const arrowDepth = graphics::arrowDepth;

  pair<m2::PointD, m2::PointD> arrowDirection;
  vector<m2::PointD> ptsTurn, ptsNextTurn;

  ptsTurn.reserve(m_turnsGeom.size());
  bool drawArrowHead = true;
  auto turnsGeomEnd = m_turnsGeom.rend();
  for (auto t = m_turnsGeom.rbegin(); t != turnsGeomEnd; ++t)
  {
    if (t->m_indexInRoute < m_relevantMatchedInfo.GetIndexInRoute())
      continue;
    ptsTurn.clear();
    if (t->m_points.empty())
      continue;
    transform(t->m_points.begin(), t->m_points.end(), back_inserter(ptsTurn), DoLeftProduct<MatrixT>(matrix));

    if (!ClipArrowBodyAndGetArrowDirection(ptsTurn, arrowDirection, t->m_turnIndex, beforeTurn, afterTurn, arrowLength))
      continue;
    if (ptsTurn.size() < 2)
      continue;

    if (!ptsNextTurn.empty())
      drawArrowHead = !MergeArrows(ptsTurn, ptsNextTurn, beforeTurn + afterTurn, arrowLength);

    graphics::Pen::Info const outlineInfo(m_arrowColor, arrowBodyWidth);
    uint32_t const outlineId = dlScreen->mapInfo(outlineInfo);
    dlScreen->drawPath(&ptsTurn[0], ptsTurn.size(), 0, outlineId, arrowDepth);

    if (drawArrowHead)
      DrawArrowTriangle(dlScreen, arrowDirection, arrowWidth, arrowLength, m_arrowColor, arrowDepth);
    ptsNextTurn = ptsTurn;
  }
}
*/

/// @todo there are some ways to optimize the code bellow.
/// 1. Call CreateDisplayListArrows only after passing the next arrow while driving
/// 2. Use several closest segments intead of one to recreate Display List for the most part of the track
///
//void RouteTrack::CreateDisplayList(graphics::Screen * dlScreen, MatrixT const & matrix, bool isScaleChanged,
//                                   int drawScale, double visualScale,
//                                   location::RouteMatchingInfo const & matchingInfo) const
//{
//  if (HasDisplayLists() && !isScaleChanged &&
//      m_relevantMatchedInfo.GetPosition() == matchingInfo.GetPosition())
//    return;

//  PolylineD const & fullPoly = GetPolyline();
//  size_t const formerIndex = m_relevantMatchedInfo.GetIndexInRoute();

//  if (matchingInfo.IsMatched())
//    m_relevantMatchedInfo = matchingInfo;
//  size_t const currentIndex = m_relevantMatchedInfo.GetIndexInRoute();

//  size_t const fullPolySz = fullPoly.GetSize();
//  if (currentIndex + 2 >= fullPolySz || fullPolySz < 2)
//  {
//    DeleteDisplayList();
//    DeleteClosestSegmentDisplayList();
//    return;
//  }
//  DeleteClosestSegmentDisplayList();
//  auto const curSegIter = fullPoly.Begin() + currentIndex;

//  //the most part of the route and symbols
//  if (formerIndex != currentIndex ||
//      !HasDisplayLists() || isScaleChanged)
//  {
//    DeleteDisplayList();
//    dlScreen->beginFrame();

//    graphics::DisplayList * dList = dlScreen->createDisplayList();
//    dlScreen->setDisplayList(dList);
//    SetDisplayList(dList);

//    PolylineD mostPartPoly(curSegIter + 1, fullPoly.End());
//    PointContainerT ptsMostPart;
//    ptsMostPart.reserve(mostPartPoly.GetSize());
//    TransformAndSymplifyPolyline(mostPartPoly, matrix, GetMainWidth(), ptsMostPart);
//    CreateDisplayListPolyline(dlScreen, ptsMostPart);
    
//    PolylineD sym(vector<m2::PointD>({fullPoly.Front(), fullPoly.Back()}));
//    PointContainerT ptsSym;
//    TransformPolyline(sym, matrix, ptsSym);
//    CreateDisplayListSymbols(dlScreen, ptsSym);

//    //arrows on the route
//    if (drawScale >= scales::GetNavigationScale())
//      CreateDisplayListArrows(dlScreen, matrix, visualScale);
//  }
//  else
//    dlScreen->beginFrame();

//  //closest route segment
//  m_closestSegmentDL = dlScreen->createDisplayList();
//  dlScreen->setDisplayList(m_closestSegmentDL);
//  PolylineD closestPoly(m_relevantMatchedInfo.IsMatched() ?
//          vector<m2::PointD>({m_relevantMatchedInfo.GetPosition(), fullPoly.GetPoint(currentIndex + 1)}) :
//          vector<m2::PointD>({fullPoly.GetPoint(currentIndex), fullPoly.GetPoint(currentIndex + 1)}));
//  PointContainerT pts;
//  pts.reserve(closestPoly.GetSize());
//  TransformPolyline(closestPoly, matrix, pts);
//  CreateDisplayListPolyline(dlScreen, pts);

//  dlScreen->setDisplayList(0);
//  dlScreen->endFrame();
//}

void RouteTrack::DeleteClosestSegmentDisplayList() const
{
//  delete m_closestSegmentDL;
//  m_closestSegmentDL = nullptr;
}

//void RouteTrack::Draw(graphics::Screen * pScreen, MatrixT const & matrix) const
//{
//  Track::Draw(pScreen, matrix);
//  pScreen->drawDisplayList(m_closestSegmentDL, matrix);
//}

void RouteTrack::AddClosingSymbol(bool isBeginSymbol, string const & symbolName, dp::Anchor pos, double depth)
{
  if (isBeginSymbol)
    m_beginSymbols.push_back(ClosingSymbol(symbolName, pos, depth));
  else
    m_endSymbols.push_back(ClosingSymbol(symbolName, pos, depth));
}

//void RouteTrack::CreateDisplayListSymbols(graphics::Screen * dlScreen, PointContainerT const & pts) const
//{
//  ASSERT(!pts.empty(), ());
//  if (!m_beginSymbols.empty() || !m_endSymbols.empty())
//  {
//    m2::PointD pivot = pts.front();
//    auto symDrawer = [&dlScreen, &pivot] (ClosingSymbol const & symbol)
//    {
//      dlScreen->drawSymbol(pivot, symbol.m_iconName, symbol.m_position, symbol.m_depth);
//    };

//    for_each(m_beginSymbols.begin(), m_beginSymbols.end(), symDrawer);

//    pivot = pts.back();
//    for_each(m_endSymbols.begin(), m_endSymbols.end(), symDrawer);
//  }
//}
