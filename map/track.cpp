#include "map/track.hpp"

#include "indexer/mercator.hpp"

#include "drape/color.hpp"

#include "geometry/distance_on_sphere.hpp"

Track::Track(Track::PolylineD const & polyline, Track::Params const & p)
  : m_polyline(polyline)
  , m_params(p)
{
  ASSERT_GREATER(m_polyline.GetSize(), 1, ());
}

string const & Track::GetName() const
{
  return m_params.m_name;
}

m2::RectD Track::GetLimitRect() const
{
  return m_polyline.GetLimitRect();
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

size_t Track::GetLayerCount() const
{
  return m_params.m_colors.size();
}

dp::Color const & Track::GetColor(size_t layerIndex) const
{
  return m_params.m_colors[layerIndex].m_color;
}

float Track::GetWidth(size_t layerIndex) const
{
  return m_params.m_colors[layerIndex].m_lineWidth;
}

float Track::GetLayerDepth(size_t layerIndex) const
{
  return 0 + layerIndex * 10;
}

size_t Track::GetPointCount() const
{
  return m_polyline.GetSize();
}

m2::PointD const & Track::GetPoint(size_t pointIndex) const
{
  return m_polyline.GetPoint(pointIndex);
}
