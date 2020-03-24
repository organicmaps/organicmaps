#include "map/track.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/user_mark_id_storage.hpp"

#include "geometry/mercator.hpp"

#include <utility>

Track::Track(kml::TrackData && data, bool interactive)
  : Base(data.m_id == kml::kInvalidTrackId ? UserMarkIdStorage::Instance().GetNextTrackId() : data.m_id)
  , m_data(std::move(data))
{
  m_data.m_id = GetId();
  CHECK_GREATER(m_data.m_pointsWithAltitudes.size(), 1, ());
  if (interactive && HasAltitudes())
    CacheDataForInteraction();
}

void Track::CacheDataForInteraction()
{
  m_interactionData = InteractionData();
  GetLengthsImpl(m_interactionData->m_lengths);
  m_interactionData->m_limitRect = GetLimitRectImpl();
}

void Track::GetLengthsImpl(std::vector<double> & lengths) const
{
  lengths.resize(m_data.m_pointsWithAltitudes.size());
  lengths[0] = 0.0;
  for (size_t i = 1; i < m_data.m_pointsWithAltitudes.size(); ++i)
  {
    auto const & pt1 = m_data.m_pointsWithAltitudes[i - 1].GetPoint();
    auto const & pt2 = m_data.m_pointsWithAltitudes[i].GetPoint();
    auto const segmentLength = mercator::DistanceOnEarth(pt1, pt2);
    lengths[i] = lengths[i - 1] + segmentLength;
  }
}

m2::RectD Track::GetLimitRectImpl() const
{
  m2::RectD limitRect;
  for (auto const & pt : m_data.m_pointsWithAltitudes)
    limitRect.Add(pt.GetPoint());
  return limitRect;
}

bool Track::HasAltitudes() const
{
  bool hasNonDefaultAltitude = false;
  for (auto const & pt : m_data.m_pointsWithAltitudes)
  {
    if (pt.GetAltitude() == geometry::kInvalidAltitude)
      return false;
    hasNonDefaultAltitude |= pt.GetAltitude() != geometry::kDefaultAltitudeMeters;
  }

  return hasNonDefaultAltitude;
}

std::string Track::GetName() const
{
  return GetPreferredBookmarkStr(m_data.m_name);
}

m2::RectD Track::GetLimitRect() const
{
  if (m_interactionData)
    return m_interactionData->m_limitRect;
  return GetLimitRectImpl();
}

double Track::GetLengthMeters() const
{
  if (m_interactionData)
    return m_interactionData->m_lengths.back();

  return GetLengthMeters(m_data.m_pointsWithAltitudes.size() - 1);
}

double Track::GetLengthMeters(size_t pointIndex) const
{
  CHECK_LESS(pointIndex, m_data.m_pointsWithAltitudes.size(),
             (pointIndex, m_data.m_pointsWithAltitudes.size()));

  if (m_interactionData)
    return m_interactionData->m_lengths[pointIndex];

  double length = 0.0;
  for (size_t i = 1; i <= pointIndex; ++i)
  {
    auto const & pt1 = m_data.m_pointsWithAltitudes[i - 1].GetPoint();
    auto const & pt2 = m_data.m_pointsWithAltitudes[i].GetPoint();
    auto const segmentLength = mercator::DistanceOnEarth(pt1, pt2);
    length += segmentLength;
  }
  return length;
}

bool Track::IsInteractive() const
{
  return m_interactionData.has_value();
}

df::DepthLayer Track::GetDepthLayer() const
{
  return df::DepthLayer::UserLineLayer;
}

size_t Track::GetLayerCount() const
{
  return m_data.m_layers.size();
}

dp::Color Track::GetColor(size_t layerIndex) const
{
  CHECK_LESS(layerIndex, m_data.m_layers.size(), ());
  return dp::Color(m_data.m_layers[layerIndex].m_color.m_rgba);
}

float Track::GetWidth(size_t layerIndex) const
{
  CHECK_LESS(layerIndex, m_data.m_layers.size(), ());
  return static_cast<float>(m_data.m_layers[layerIndex].m_lineWidth);
}

float Track::GetDepth(size_t layerIndex) const
{
  return layerIndex * 10;
}

std::vector<m2::PointD> Track::GetPoints() const
{
  std::vector<m2::PointD> result;
  result.reserve(m_data.m_pointsWithAltitudes.size());
  for (auto const & pt : m_data.m_pointsWithAltitudes)
    result.push_back(pt.GetPoint());

  return result;
}

std::vector<geometry::PointWithAltitude> const & Track::GetPointsWithAltitudes() const
{
  return m_data.m_pointsWithAltitudes;
}

void Track::Attach(kml::MarkGroupId groupId)
{
  ASSERT_EQUAL(m_groupID, kml::kInvalidMarkGroupId, ());
  m_groupID = groupId;
}

void Track::Detach()
{
  m_groupID = kml::kInvalidMarkGroupId;
}

void Track::SetSelectionMarkId(kml::MarkId markId)
{
  m_selectionMarkId = markId;
}

bool Track::GetPoint(double distanceInMeters, m2::PointD & pt) const
{
  CHECK_GREATER_OR_EQUAL(distanceInMeters, 0.0, (distanceInMeters));

  if (m_interactionData)
    return GetPointImpl(m_interactionData->m_lengths, distanceInMeters, pt);

  std::vector<double> lengths;
  GetLengthsImpl(lengths);
  return GetPointImpl(lengths, distanceInMeters, pt);
}

bool Track::GetPointImpl(std::vector<double> const & lengths, double distanceInMeters,
                         m2::PointD & pt) const
{
  double const kEpsMeters = 1e-2;
  if (base::AlmostEqualAbs(distanceInMeters, lengths.front(), kEpsMeters))
  {
    pt = m_data.m_pointsWithAltitudes.front().GetPoint();
    return true;
  }

  if (base::AlmostEqualAbs(distanceInMeters, lengths.back(), kEpsMeters))
  {
    pt = m_data.m_pointsWithAltitudes.back().GetPoint();
    return true;
  }

  auto const it = std::lower_bound(lengths.begin(), lengths.end(), distanceInMeters);
  if (it == lengths.end())
    return false;

  auto const pointIndex = std::distance(lengths.begin(), it);
  auto const length = *it;

  auto const segmentLength = length - lengths[pointIndex - 1];
  auto const k = (segmentLength - (length - distanceInMeters)) / segmentLength;

  auto const & pt1 = m_data.m_pointsWithAltitudes[pointIndex - 1].GetPoint();
  auto const & pt2 = m_data.m_pointsWithAltitudes[pointIndex].GetPoint();
  pt = pt1 + (pt2 - pt1) * k;

  return true;
}
