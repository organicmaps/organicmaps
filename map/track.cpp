#include "track.hpp"

#include "../indexer/mercator.hpp"

#include "../graphics/screen.hpp"
#include "../graphics/pen.hpp"
#include "../graphics/depth_constants.hpp"
#include "../graphics/display_list.hpp"

#include "../geometry/distance.hpp"
#include "../geometry/simplification.hpp"
#include "../geometry/distance_on_sphere.hpp"

#include "../base/timer.hpp"
#include "../base/logging.hpp"


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

void Track::DeleteDisplayList() const
{
  if (m_dList)
  {
    delete m_dList;
    m_dList = 0;
  }
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

void Track::CreateDisplayList(graphics::Screen * dlScreen, MatrixT const & matrix) const
{
  DeleteDisplayList();

  m_dList = dlScreen->createDisplayList();

  dlScreen->beginFrame();
  dlScreen->setDisplayList(m_dList);

  graphics::Pen::Info info(m_color, m_width);
  uint32_t resId = dlScreen->mapInfo(info);

  typedef buffer_vector<m2::PointD, 32> PointContainerT;
  size_t const count = m_polyline.GetSize();

  PointContainerT pts1(count);
  transform(m_polyline.Begin(), m_polyline.End(), pts1.begin(), DoLeftProduct<MatrixT>(matrix));

  PointContainerT pts2;
  pts2.reserve(count);
  SimplifyDP(pts1.begin(), pts1.end(), math::sqr(m_width),
             m2::DistanceToLineSquare<m2::PointD>(), MakeBackInsertFunctor(pts2));

  if (IsMarked())
  {
    graphics::Pen::Info outlineInfo(m_outlineColor, m_width + 2 * m_outlineWidth);
    uint32_t outlineId = dlScreen->mapInfo(outlineInfo);
    dlScreen->drawPath(pts2.data(), pts2.size(), 0, outlineId, graphics::tracksDepth);
  }

  dlScreen->drawPath(pts2.data(), pts2.size(), 0, resId, graphics::tracksDepth);

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

double Track::GetShortestSquareDistance(m2::PointD const & point) const
{
  return m_polyline.GetShortestSquareDistance(point);
}

void Track::Swap(Track & rhs)
{
  swap(m_isVisible, rhs.m_isVisible);
  swap(m_width, rhs.m_width);
  swap(m_color, rhs.m_color);
  swap(m_rect, rhs.m_rect);
  swap(m_isMarked, rhs.m_isMarked);
  swap(m_outlineColor, rhs.m_outlineColor);
  swap(m_outlineWidth, rhs.m_outlineWidth);

  m_name.swap(rhs.m_name);
  m_polyline.Swap(rhs.m_polyline);

  DeleteDisplayList();
  rhs.DeleteDisplayList();
}
