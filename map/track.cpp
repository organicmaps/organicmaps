#include "map/track.hpp"

#include "geometry/mercator.hpp"

#include "drape/color.hpp"

#include "geometry/distance_on_sphere.hpp"

namespace
{
df::LineID GetNextUserLineId()
{
  static std::atomic<uint32_t> nextLineId(0);
  return static_cast<df::LineID>(++nextLineId);
}
}  // namespace

Track::Track(Track::PolylineD const & polyline, Track::Params const & p)
  : df::UserLineMark(GetNextUserLineId())
  , m_polyline(polyline)
  , m_params(p)
  , m_groupID(0)
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

  auto i = m_polyline.Begin();
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

df::RenderState::DepthLayer Track::GetDepthLayer() const
{
  return df::RenderState::UserLineLayer;
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

float Track::GetDepth(size_t layerIndex) const
{
  return layerIndex * 10;
}

std::vector<m2::PointD> const & Track::GetPoints() const
{
  return m_polyline.GetPoints();
}

void Track::Attach(df::MarkGroupID groupId)
{
  ASSERT(!m_groupID, ());
  m_groupID = groupId;
}

void Track::Detach()
{
  m_groupID = 0;
}
