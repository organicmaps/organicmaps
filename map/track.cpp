#include "track.hpp"

#include "../graphics/screen.hpp"
#include "../graphics/pen.hpp"
#include "../graphics/depth_constants.hpp"
#include "../graphics/display_list.hpp"

#include "../geometry/distance.hpp"

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

  /// @todo add simplification
  vector<m2::PointD> pts(m_polyline.GetSize());
  transform(m_polyline.Begin(), m_polyline.End(), pts.begin(), DoLeftProduct<MatrixT>(matrix));

  dlScreen->drawPath(pts.data(), pts.size(), 0, resId, graphics::tracksDepth);

  dlScreen->setDisplayList(0);
  dlScreen->endFrame();
}

double Track::GetShortestSquareDistance(m2::PointD const & point) const
{
  double res = numeric_limits<double>::max();
  m2::DistanceToLineSquare<m2::PointD> d;

  typedef PolylineD::IterT IterT;
  IterT i = m_polyline.Begin();
  for (IterT j = i+1; j != m_polyline.End(); ++i, ++j)
  {
    d.SetBounds(*i, *j);
    res = min(res, d(point));
  }

  return res;
}

void Track::Swap(Track & rhs)
{
  swap(m_isVisible, rhs.m_isVisible);
  swap(m_width, rhs.m_width);
  swap(m_color, rhs.m_color);
  swap(m_rect, rhs.m_rect);

  m_name.swap(rhs.m_name);
  m_polyline.Swap(rhs.m_polyline);

  DeleteDisplayList();
  rhs.DeleteDisplayList();
}
