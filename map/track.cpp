#include "map/track.hpp"

#include "geometry/mercator.hpp"

#include "drape/color.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "platform/platform.hpp"

#include "base/string_utils.hpp"

namespace
{
static const std::string kLastLineId = "LastLineId";

uint64_t LoadLastLineId()
{
  uint64_t lastId;
  std::string val;
  if (GetPlatform().GetSecureStorage().Load(kLastLineId, val) && strings::to_uint64(val, lastId))
    return lastId;
  return 0;
}

void SaveLastLineId(uint64_t lastId)
{
  GetPlatform().GetSecureStorage().Save(kLastLineId, strings::to_string(lastId));
}

df::LineID GetNextUserLineId(bool reset = false)
{
  static std::atomic<uint64_t> lastLineId(LoadLastLineId());

  if (reset)
  {
    SaveLastLineId(0);
    lastLineId = 0;
    return df::kInvalidLineId;
  }

  auto const id = static_cast<df::LineID>(++lastLineId);
  SaveLastLineId(lastLineId);
  return id;
}

}  // namespace

Track::Track(kml::TrackData const & data)
  : df::UserLineMark(data.m_id == df::kInvalidLineId ? GetNextUserLineId() : data.m_id)
  , m_data(data)
  , m_groupID(0)
{
  m_data.m_id = GetId();
  ASSERT_GREATER(m_data.m_points.size(), 1, ());
}

// static
void Track::ResetLastId()
{
  UNUSED_VALUE(GetNextUserLineId(true /* reset */));
}

string Track::GetName() const
{
  return kml::GetDefaultStr(m_data.m_name);
}

m2::RectD Track::GetLimitRect() const
{
  m2::RectD rect;
  for (auto const & point : m_data.m_points)
    rect.Add(point);
  return rect;
}

double Track::GetLengthMeters() const
{
  double res = 0.0;

  auto it = m_data.m_points.begin();
  double lat1 = MercatorBounds::YToLat(it->y);
  double lon1 = MercatorBounds::XToLon(it->x);
  for (++it; it != m_data.m_points.end(); ++it)
  {
    double const lat2 = MercatorBounds::YToLat(it->y);
    double const lon2 = MercatorBounds::XToLon(it->x);
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
  return m_data.m_layers.size();
}

dp::Color Track::GetColor(size_t layerIndex) const
{
  return dp::Color(m_data.m_layers[layerIndex].m_color.m_rgba);
}

float Track::GetWidth(size_t layerIndex) const
{
  return static_cast<float>(m_data.m_layers[layerIndex].m_lineWidth);
}

float Track::GetDepth(size_t layerIndex) const
{
  return layerIndex * 10;
}

std::vector<m2::PointD> const & Track::GetPoints() const
{
  return m_data.m_points;
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
