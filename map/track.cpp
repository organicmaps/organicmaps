#include "map/track.hpp"

#include "indexer/mercator.hpp"

#include "drape/color.hpp"
#include "drape/drape_global.hpp"

#include "geometry/distance.hpp"
#include "geometry/simplification.hpp"
#include "geometry/distance_on_sphere.hpp"

#include "base/timer.hpp"
#include "base/logging.hpp"

#include "platform/location.hpp"


Track::~Track()
{
  ///@TODO UVR
  //DeleteDisplayList();
}

Track * Track::CreatePersistent()
{
  Track * p = new Track();
  Swap(*p);
  return p;
}

float Track::GetMainWidth() const
{
  ASSERT(!m_outlines.empty(), ());
  return m_outlines.back().m_lineWidth;
}

dp::Color const & Track::GetMainColor() const
{
  ASSERT(!m_outlines.empty(), ());
  return m_outlines.back().m_color;
}

///@TODO UVR
//void Track::DeleteDisplayList() const
//{
//  if (m_dList)
//  {
//    delete m_dList;
//    m_dList = nullptr;
//  }
//}

void Track::AddOutline(TrackOutline const * outline, size_t arraySize)
{
  m_outlines.reserve(m_outlines.size() + arraySize);
  for_each(outline, outline + arraySize, MakeBackInsertFunctor(m_outlines));
  sort(m_outlines.begin(), m_outlines.end(), [] (TrackOutline const & l, TrackOutline const & r)
  {
    return l.m_lineWidth > r.m_lineWidth;
  });
}

///@TODO UVR
//void Track::Draw(graphics::Screen * pScreen, MatrixT const & matrix) const
//{
//  pScreen->drawDisplayList(m_dList, matrix);
//}

///@TODO UVR
//void Track::CreateDisplayListPolyline(graphics::Screen * dlScreen, PointContainerT const & pts) const
//{
//  double baseDepthTrack = graphics::tracksDepth - 10 * m_outlines.size();
//  for (TrackOutline const & outline : m_outlines)
//  {
//    graphics::Pen::Info const outlineInfo(outline.m_color, outline.m_lineWidth);
//    uint32_t const outlineId = dlScreen->mapInfo(outlineInfo);
//    dlScreen->drawPath(pts.data(), pts.size(), 0, outlineId, baseDepthTrack);
//    baseDepthTrack += 10;
//  }
//}

///@TODO UVR
//void Track::CreateDisplayList(graphics::Screen * dlScreen, MatrixT const & matrix, bool isScaleChanged,
//                              int, double, location::RouteMatchingInfo const &) const
//{
//  if (HasDisplayLists() && !isScaleChanged)
//    return;

//  DeleteDisplayList();

//  m_dList = dlScreen->createDisplayList();
//  dlScreen->beginFrame();
//  dlScreen->setDisplayList(m_dList);

//  PointContainerT pts;
//  pts.reserve(m_polyline.GetSize());
//  TransformAndSymplifyPolyline(m_polyline, matrix, GetMainWidth(), pts);
//  CreateDisplayListPolyline(dlScreen, pts);

//  dlScreen->setDisplayList(0);
//  dlScreen->endFrame();
//}

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
  ///@TODO UVR
  //swap(m_rect, rhs.m_rect);
  //swap(m_outlines, rhs.m_outlines);
  //m_name.swap(rhs.m_name);
  //m_polyline.Swap(rhs.m_polyline);

  //DeleteDisplayList();
  //rhs.DeleteDisplayList();
}

///@TODO UVR
//void Track::CleanUp() const
//{
//  DeleteDisplayList();
//}

//bool Track::HasDisplayLists() const
//{
//  return m_dList != nullptr;
//}

void TransformPolyline(Track::PolylineD const & polyline, MatrixT const & matrix, PointContainerT & pts)
{
  pts.resize(polyline.GetSize());
  transform(polyline.Begin(), polyline.End(), pts.begin(), DoLeftProduct<MatrixT>(matrix));
}

void TransformAndSymplifyPolyline(Track::PolylineD const & polyline, MatrixT const & matrix, double width, PointContainerT & pts)
{
  PointContainerT pts1(polyline.GetSize());
  TransformPolyline(polyline, matrix, pts1);
  SimplifyDP(pts1.begin(), pts1.end(), width,
             m2::DistanceToLineSquare<m2::PointD>(), MakeBackInsertFunctor(pts));
}
