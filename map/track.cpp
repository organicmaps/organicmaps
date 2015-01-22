#include "track.hpp"

#include "../indexer/mercator.hpp"

#include "../graphics/screen.hpp"
#include "../graphics/pen.hpp"
#include "../graphics/depth_constants.hpp"
#include "../graphics/display_list.hpp"
#include "../graphics/defines.hpp"

#include "../geometry/distance.hpp"
#include "../geometry/simplification.hpp"
#include "../geometry/distance_on_sphere.hpp"

#include "../base/timer.hpp"
#include "../base/logging.hpp"

#include "../indexer/scales.hpp"

typedef buffer_vector<m2::PointD, 32> PointContainerT;

pair<m2::PointD, m2::PointD> shiftArrow(pair<m2::PointD, m2::PointD> const & arrowDirection)
{
  return pair<m2::PointD, m2::PointD>(arrowDirection.first - (arrowDirection.second - arrowDirection.first),
                                      arrowDirection.first);
}

bool clipArrowBodyAndGetArrowDirection(vector<m2::PointD> & ptsTurn, pair<m2::PointD, m2::PointD> & arrowDirection,
                                       size_t turnIndex, double beforeTurn, double afterTurn, double arrowLength)
{
  size_t const ptsTurnSz = ptsTurn.size();
  ASSERT(turnIndex < ptsTurnSz, ());

  /// Clipping after turnIndex
  size_t i = turnIndex;
  double len = 0, vLen = 0;
  while (len < afterTurn)
  {
    if (i >= ptsTurnSz - 2)
      return false;
    vLen = ptsTurn[i + 1].Length(ptsTurn[i]);
    len += vLen;
    i += 1;
  }
  if (my::AlmostEqual(vLen, 0.))
    return false;

  double lenForArrow = len - afterTurn;
  double vLenForArrow = lenForArrow;
  size_t j = i;
  while (lenForArrow < arrowLength)
  {
    if (j >= ptsTurnSz - 2)
      return false;
    vLenForArrow = ptsTurn[j + 1].Length(ptsTurn[j]);
    lenForArrow +=  vLenForArrow;
    j += 1;
  }
  if (m2::AlmostEqual(ptsTurn[j - 1], ptsTurn[j]))
    return false;
  m2::PointD arrowEnd = m2::PointAtSegment(ptsTurn[j - 1], ptsTurn[j], vLenForArrow - (lenForArrow - arrowLength));

  if (my::AlmostEqual(len, afterTurn))
    ptsTurn.resize(i + 1);
  else
  {
    if (!m2::AlmostEqual(ptsTurn[i], ptsTurn[i - 1]))
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
  arrowDirection = shiftArrow(arrowDirection);

  /// Clipping before turnIndex
  i = turnIndex;
  len = 0;
  while (len < beforeTurn)
  {
    if (i <= 1)
      return false;
    vLen = ptsTurn[i - 1].Length(ptsTurn[i]);
    len += vLen;
    i -= 1;
  }
  if (my::AlmostEqual(vLen, 0.))
    return false;

  if (my::AlmostEqual(len, beforeTurn))
  {
    if (i != 0)
    {
      ptsTurn.erase(ptsTurn.begin(), ptsTurn.begin() + i);
      return true;
    }
  }
  else
  {
    if (!m2::AlmostEqual(ptsTurn[i], ptsTurn[i + 1]))
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

void drawArrowTriangle(graphics::Screen * dlScreen, pair<m2::PointD, m2::PointD> const & arrowDirection,
                       double arrowWidth, double arrowLength, graphics::Color arrowColor, double arrowDepth)
{
  ASSERT(dlScreen, ());
  m2::PointD p1, p2, p3;

  m2::ArrowPoints(arrowDirection.first, arrowDirection.second, arrowWidth, arrowLength, p1, p2, p3);
  vector<m2::PointF> arrow = {p1, p2, p3};
  dlScreen->drawConvexPolygon(&arrow[0], arrow.size(), arrowColor, arrowDepth);
}

Track::~Track()
{
  DeleteDisplayList();
}

Track * Track::CreatePersistent()
{
  Track * p = new Track();
  p->Swap(*this);
  return p;
}

float Track::GetMainWidth() const
{
  ASSERT(!m_outlines.empty(), ());
  return m_outlines.back().m_lineWidth;
}

const graphics::Color & Track::GetMainColor() const
{
  ASSERT(!m_outlines.empty(), ());
  return m_outlines.back().m_color;
}

void Track::DeleteDisplayList() const
{
  if (m_dList)
  {
    delete m_dList;
    m_dList = nullptr;
  }
}

void Track::AddOutline(TrackOutline const * outline, size_t arraySize)
{
  m_outlines.reserve(m_outlines.size() + arraySize);
  for_each(outline, outline + arraySize, MakeBackInsertFunctor(m_outlines));
  sort(m_outlines.begin(), m_outlines.end(), [] (TrackOutline const & l, TrackOutline const & r)
  {
    return l.m_lineWidth > r.m_lineWidth;
  });
}

void Track::AddClosingSymbol(bool isBeginSymbol, string const & symbolName, graphics::EPosition pos, double depth)
{
  if (isBeginSymbol)
    m_beginSymbols.push_back(ClosingSymbol(symbolName, pos, depth));
  else
    m_endSymbols.push_back(ClosingSymbol(symbolName, pos, depth));
}

void Track::Draw(graphics::Screen * pScreen, MatrixT const & matrix) const
{
  pScreen->drawDisplayList(m_dList, matrix);
}

namespace
{

template <class T> class DoLeftProduct
{
  T const & m_t;
public:
  DoLeftProduct(T const & t) : m_t(t) {}
  template <class X> X operator() (X const & x) const { return x * m_t; }
};

}

void Track::CreateDisplayListArrows(graphics::Screen * dlScreen, MatrixT const & matrix, double visualScale) const
{
  double const beforeTurn = 13. * visualScale;
  double const afterTurn = 13. * visualScale;
  double const arrowWidth = 10. * visualScale;
  double const arrowLength = 19. * visualScale;
  double const arrowBodyWidth = 8. * visualScale;
  graphics::Color const arrowColor(graphics::Color(0, 0, 128, 255));
  double const arrowDepth = graphics::arrowDepth;

  pair<m2::PointD, m2::PointD> arrowDirection;
  vector<m2::PointD> ptsTurn;
  ptsTurn.reserve(m_turnsGeom.size());
  for (routing::turns::TurnGeom const & t : m_turnsGeom)
  {
    ptsTurn.clear();
    if (t.m_points.empty())
      continue;
    transform(t.m_points.begin(), t.m_points.end(), back_inserter(ptsTurn), DoLeftProduct<MatrixT>(matrix));

    if (!clipArrowBodyAndGetArrowDirection(ptsTurn, arrowDirection, t.m_turnIndex, beforeTurn, afterTurn, arrowLength))
      continue;
    size_t const ptsTurnSz = ptsTurn.size();
    if (ptsTurnSz < 2)
      continue;

    graphics::Pen::Info const outlineInfo(arrowColor, arrowBodyWidth);
    uint32_t const outlineId = dlScreen->mapInfo(outlineInfo);
    dlScreen->drawPath(&ptsTurn[0], ptsTurnSz, 0, outlineId, arrowDepth);

    drawArrowTriangle(dlScreen, arrowDirection, arrowWidth, arrowLength, arrowColor, arrowDepth);
  }
}

void Track::CreateDisplayList(graphics::Screen * dlScreen, MatrixT const & matrix, int drawScale, double visualScale) const
{
  DeleteDisplayList();

  m_dList = dlScreen->createDisplayList();

  dlScreen->beginFrame();
  dlScreen->setDisplayList(m_dList);

  size_t const count = m_polyline.GetSize();

  PointContainerT pts1(count);
  transform(m_polyline.Begin(), m_polyline.End(), pts1.begin(), DoLeftProduct<MatrixT>(matrix));

  PointContainerT pts2;
  pts2.reserve(count);
  SimplifyDP(pts1.begin(), pts1.end(), GetMainWidth(),
             m2::DistanceToLineSquare<m2::PointD>(), MakeBackInsertFunctor(pts2));

  double baseDepthTrack = graphics::tracksDepth - 10 * m_outlines.size();
  for (TrackOutline const & outline : m_outlines)
  {
    graphics::Pen::Info const outlineInfo(outline.m_color, outline.m_lineWidth);
    uint32_t const outlineId = dlScreen->mapInfo(outlineInfo);
    dlScreen->drawPath(pts2.data(), pts2.size(), 0, outlineId, baseDepthTrack);
    baseDepthTrack += 10;
  }

  if (!m_beginSymbols.empty() || !m_endSymbols.empty())
  {
    m2::PointD pivot = pts2.front();
    auto symDrawer = [&dlScreen, &pivot] (ClosingSymbol const & symbol)
    {
      dlScreen->drawSymbol(pivot, symbol.m_iconName, symbol.m_position, symbol.m_depth);
    };

    for_each(m_beginSymbols.begin(), m_beginSymbols.end(), symDrawer);

    pivot = pts2.back();
    for_each(m_endSymbols.begin(), m_endSymbols.end(), symDrawer);
  }

  if (drawScale >= scales::GetNavigationScale())
    CreateDisplayListArrows(dlScreen, matrix, visualScale);

  dlScreen->setDisplayList(0);
  dlScreen->endFrame();
}

double Track::GetLengthMeters() const
{
  double res = 0.0;

  PolylineD::TIter i = m_polyline.Begin();
  double lat1 = MercatorBounds::YToLat(i->y);
  double lon1 = MercatorBounds::XToLon(i->x);
  for (++i; i != m_polyline.End(); ++i)
  {
    double const lat2 = MercatorBounds::YToLat(i->y);
    double const lon2 = MercatorBounds::XToLon(i->x);
    res += ms::DistanceOnEarth(lat1, lon1, lat2, lon2);
    lat1 = lat2;
    lon1 = lon2;
  }

  return res;
}

void Track::Swap(Track & rhs)
{
  swap(m_isVisible, rhs.m_isVisible);
  swap(m_rect, rhs.m_rect);
  swap(m_outlines, rhs.m_outlines);
  swap(m_beginSymbols, rhs.m_beginSymbols);
  swap(m_endSymbols, rhs.m_endSymbols);

  m_name.swap(rhs.m_name);
  m_polyline.Swap(rhs.m_polyline);
  m_turnsGeom.swap(rhs.m_turnsGeom);

  DeleteDisplayList();
  rhs.DeleteDisplayList();
}
