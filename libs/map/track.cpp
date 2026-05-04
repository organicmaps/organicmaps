#include "map/track.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/user_mark_id_storage.hpp"

#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"

#include <algorithm>

Track::Track(kml::TrackData && data)
  : Base(data.m_id == kml::kInvalidTrackId ? UserMarkIdStorage::Instance().GetNextTrackId() : data.m_id)
  , m_data(std::move(data))
{
  m_data.m_id = GetId();
  CHECK(m_data.m_geometry.IsValid(), ());
}

void Track::CacheDataForInteraction() const
{
  m_interactionData = InteractionData();
  m_interactionData->m_lengths = GetLengthsImpl();
  m_interactionData->m_limitRect = GetLimitRectImpl();
}

std::vector<Track::Lengths> Track::GetLengthsImpl() const
{
  double distance = 0;
  std::vector<Lengths> lengths;
  for (auto const & line : m_data.m_geometry.m_lines)
  {
    ASSERT(!line.empty(), ());
    Lengths lineLengths;
    lineLengths.emplace_back(distance);
    for (size_t j = 1; j < line.size(); ++j)
    {
      auto const & pt1 = line[j - 1].GetPoint();
      auto const & pt2 = line[j].GetPoint();
      distance += mercator::DistanceOnEarth(pt1, pt2);
      lineLengths.emplace_back(distance);
    }
    lengths.emplace_back(std::move(lineLengths));
  }
  return lengths;
}

m2::RectD Track::GetLimitRectImpl() const
{
  m2::RectD limitRect;
  for (auto const & line : m_data.m_geometry.m_lines)
    for (auto const & pt : line)
      limitRect.Add(pt.GetPoint());
  return limitRect;
}

bool Track::HasAltitudes() const
{
  bool hasNonDefaultAltitude = false;

  for (auto const & line : m_data.m_geometry.m_lines)
  {
    for (auto const & pt : line)
    {
      if (pt.GetAltitude() == geometry::kInvalidAltitude)
        return false;
      if (!hasNonDefaultAltitude && pt.GetAltitude() != geometry::kDefaultAltitudeMeters)
        hasNonDefaultAltitude = true;
    }
  }

  return hasNonDefaultAltitude;
}

std::string Track::GetName() const
{
  return GetPreferredBookmarkStr(m_data.m_name);
}

void Track::SetName(std::string const & name)
{
  m_isDirty = true;
  kml::SetDefaultStr(m_data.m_name, name);
}

std::string Track::GetDescription() const
{
  return GetPreferredBookmarkStr(m_data.m_description);
}

void Track::SetDescription(std::string const & description)
{
  m_isDirty = true;
  kml::SetDefaultStr(m_data.m_description, description);
}

void Track::SetData(kml::TrackData const & data)
{
  m_isDirty = true;
  m_data = data;

  m_elevationInfo.reset();
  m_interactionData.reset();
}

m2::RectD Track::GetLimitRect() const
{
  if (m_interactionData)
    return m_interactionData->m_limitRect;
  return GetLimitRectImpl();
}

double Track::GetLengthMeters() const
{
  if (!m_interactionData)
    CacheDataForInteraction();
  auto const & lengths = m_interactionData->m_lengths;
  return lengths.empty() ? 0 : lengths.back().back();
}

double Track::GetLengthMetersImpl(size_t lineIndex, size_t ptIndex) const
{
  if (!m_interactionData)
    CacheDataForInteraction();
  auto const & lineLengths = m_interactionData->m_lengths[lineIndex];
  return lineLengths[ptIndex];
}

void Track::UpdateSelectionInfo(m2::PointD const & tapPoint, TrackSelectionInfo & info) const
{
  // Caller sets info.m_squareDist to the max allowed squared distance (mercator). Any segment
  // whose closest point is strictly closer replaces the current best.
  if (m_interactionData && m_interactionData->m_limitRect.SquaredDistance(tapPoint) >= info.m_squareDist)
    return;

  for (size_t lineIndex = 0; lineIndex < m_data.m_geometry.m_lines.size(); ++lineIndex)
  {
    auto const & line = m_data.m_geometry.m_lines[lineIndex];
    for (size_t ptIndex = 0; ptIndex + 1 < line.size(); ++ptIndex)
    {
      m2::ParametrizedSegment<m2::PointD> seg(line[ptIndex].GetPoint(), line[ptIndex + 1].GetPoint());
      auto const closestPoint = seg.ClosestPointTo(tapPoint);
      auto const squaredDist = closestPoint.SquaredLength(tapPoint);
      if (squaredDist >= info.m_squareDist)
        continue;

      info.m_squareDist = squaredDist;
      info.m_trackId = m_data.m_id;
      info.m_trackPoint = closestPoint;

      auto const segDistInMeters = mercator::DistanceOnEarth(line[ptIndex].GetPoint(), closestPoint);
      info.m_distFromBegM = segDistInMeters + GetLengthMetersImpl(lineIndex, ptIndex);
    }
  }
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

void Track::SetColor(dp::Color color)
{
  m_isDirty = true;
  m_data.m_layers[0].m_color.m_rgba = color.GetRGBA();
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

void Track::ForEachGeometry(GeometryFnT && fn) const
{
  for (auto const & line : m_data.m_geometry.m_lines)
  {
    std::vector<m2::PointD> points;
    points.reserve(line.size());
    for (auto const & pt : line)
      points.push_back(pt.GetPoint());

    fn(std::move(points));
  }
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

m2::PointD Track::GetPoint(double distanceInMeters) const
{
  if (!m_interactionData)
    CacheDataForInteraction();

  auto const & lengths = m_interactionData->m_lengths;
  ASSERT(!lengths.empty(), ());

  for (size_t lineIndex = 0; lineIndex < lengths.size(); ++lineIndex)
  {
    auto const & lineLengths = lengths[lineIndex];
    ASSERT(!lineLengths.empty(), ());

    if (distanceInMeters > lineLengths.back())
      continue;

    if (distanceInMeters <= lineLengths.front())
      return m_data.m_geometry.m_lines[lineIndex].front().GetPoint();

    auto const it = std::upper_bound(lineLengths.begin(), lineLengths.end(), distanceInMeters);
    if (it == lineLengths.end())
      return m_data.m_geometry.m_lines[lineIndex].back().GetPoint();

    ASSERT(it != lineLengths.begin(), ());
    size_t const ptIdx = std::distance(lineLengths.begin(), it) - 1;

    auto const & line = m_data.m_geometry.m_lines[lineIndex];
    auto const segLen = lineLengths[ptIdx + 1] - lineLengths[ptIdx];
    if (segLen < 1e-9)
      return line[ptIdx].GetPoint();

    double const f = (distanceInMeters - lineLengths[ptIdx]) / segLen;
    auto const & p1 = line[ptIdx].GetPoint();
    auto const & p2 = line[ptIdx + 1].GetPoint();
    return p1 + (p2 - p1) * f;
  }

  // Distance beyond last line — return last point.
  return m_data.m_geometry.m_lines.back().back().GetPoint();
}

kml::MultiGeometry::LineT Track::GetGeometry() const
{
  kml::MultiGeometry::LineT geometry;
  for (auto const & line : m_data.m_geometry.m_lines)
    for (size_t i = 0; i < line.size(); ++i)
      geometry.emplace_back(line[i]);
  return geometry;
}

TrackStatistics Track::GetStatistics() const
{
  TrackStatistics ts;
  ts.m_length = GetLengthMeters();
  ts.CalculateDuration(m_data.m_geometry);

  if (auto const * ei = GetElevationInfo())
  {
    // Relation tracks from MWM have cleaner altitude data than raw GPS tracks.
    auto const threshold =
        (m_data.m_id == kml::kTempRelationTrackId) ? ElevationInfo::kDefThresholdMWM : ElevationInfo::kDefThresholdGPS;
    auto const altInfo = ei->CalculateAltitudesInfo(threshold);
    ts.m_ascent = altInfo.GetTotalAscent();
    ts.m_descent = altInfo.GetTotalDescent();
    ts.m_minElevation = altInfo.m_minAltitude;
    ts.m_maxElevation = altInfo.m_maxAltitude;
  }
  return ts;
}

ElevationInfo const * Track::GetElevationInfo() const
{
  if (!HasAltitudes())
    return nullptr;
  if (!m_elevationInfo)
  {
    m_elevationInfo = ElevationInfo(GetData().m_geometry.m_lines);

    // Relation tracks from MWM have cleaner altitude data than raw GPS tracks.
    if (m_data.m_id != kml::kTempRelationTrackId)
      m_elevationInfo->SmoothSlopeOutliers();
    m_elevationInfo->Simplify();
  }
  return &(*m_elevationInfo);
}

double Track::GetDurationInSeconds() const
{
  TrackStatistics ts;
  ts.CalculateDuration(m_data.m_geometry);
  return ts.m_duration;
}
