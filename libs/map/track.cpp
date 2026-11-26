#include "map/track.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/user_mark_id_storage.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <cmath>

m2::PointD TrackSourcePosition::GetPoint(Track const & track) const
{
  auto const & line = track.GetData().m_geometry.m_lines[m_line];
  return mercator::Interpolate(line[m_segment].GetPoint(), line[m_segment + 1].GetPoint(), m_fraction);
}

double TrackSourcePosition::GetDistanceMeters(Track const & track) const
{
  auto const & lengths = track.GetCumulativeLengths()[m_line];
  return std::lerp(lengths[m_segment], lengths[m_segment + 1], m_fraction);
}

Track::Track(kml::TrackData && data)
  : Base(data.m_id == kml::kInvalidTrackId ? UserMarkIdStorage::Instance().GetNextTrackId() : data.m_id)
  , m_data(std::move(data))
{
  m_data.m_id = GetId();
  CHECK(m_data.m_geometry.HasLines(), (m_data.m_id));
  ASSERT(m_data.m_geometry.IsValid(), (m_data.m_id));
}

void Track::CacheDataForInteraction() const
{
  m_interactionData = InteractionData{GetLengthsImpl(), GetLimitRectImpl()};
}

std::vector<Track::Lengths> Track::GetLengthsImpl() const
{
  // Sum like the elevation chart (ElevationInfo ctor + ForEachPoint()): each line from 0, plus the sum of previous
  // line lengths. Chart distances resolve to points here bit-exactly; any rounding difference at a gap moves the
  // selection into the next line.
  double lineOffset = 0;
  std::vector<Lengths> lengths;
  for (auto const & line : m_data.m_geometry.m_lines)
  {
    double distance = 0;
    Lengths lineLengths;
    lineLengths.emplace_back(lineOffset);
    for (size_t j = 1; j < line.size(); ++j)
    {
      distance += mercator::DistanceOnEarth(line[j - 1].GetPoint(), line[j].GetPoint());
      lineLengths.emplace_back(lineOffset + distance);
    }
    lineOffset += distance;
    lengths.emplace_back(std::move(lineLengths));
  }
  return lengths;
}

m2::RectD Track::GetLimitRectImpl() const
{
  m2::RectD limitRect;
  double x = 0.0;
  for (auto const & line : m_data.m_geometry.m_lines)
    for (auto const & pt : line)
    {
      auto const point = mercator::NearestWorldCopy(pt.GetPoint(), x);
      limitRect.Add(point);
      x = point.x;
    }
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

m2::RectD Track::GetLimitRect() const
{
  if (m_interactionData)
    return m_interactionData->m_limitRect;
  return GetLimitRectImpl();
}

double Track::GetLengthMeters() const
{
  return GetCumulativeLengths().back().back();
}

std::vector<Track::Lengths> const & Track::GetCumulativeLengths() const
{
  if (!m_interactionData)
    CacheDataForInteraction();
  return m_interactionData->m_lengths;
}

std::optional<TrackSourcePosition> Track::FindClosestSegment(m2::PointD const & tapPoint, double & squareDistance) const
{
  if (m_interactionData)
  {
    auto const & rect = m_interactionData->m_limitRect;
    if (rect.SquaredDistance(mercator::NearestWorldCopy(tapPoint, rect.Center().x)) >= squareDistance)
      return std::nullopt;
  }

  std::optional<TrackSourcePosition> result;
  for (size_t lineIndex = 0; lineIndex < m_data.m_geometry.m_lines.size(); ++lineIndex)
  {
    auto const & line = m_data.m_geometry.m_lines[lineIndex];
    for (size_t ptIndex = 0; ptIndex + 1 < line.size(); ++ptIndex)
    {
      auto const & from = line[ptIndex].GetPoint();
      auto const & to = line[ptIndex + 1].GetPoint();
      double const fraction = mercator::SegmentProjectionFraction(from, to, tapPoint);
      auto const closestPoint = mercator::Interpolate(from, to, fraction);
      auto const squaredDist = mercator::NearestWorldCopy(closestPoint, tapPoint.x).SquaredLength(tapPoint);
      if (squaredDist >= squareDistance)
        continue;

      squareDistance = squaredDist;
      result = TrackSourcePosition{lineIndex, ptIndex, fraction};
    }
  }
  return result;
}

void Track::UpdateSelectionInfo(m2::PointD const & tapPoint, TrackSelectionInfo & info) const
{
  if (auto const position = FindClosestSegment(tapPoint, info.m_squareDist))
  {
    info.m_title = GetName();
    info.m_color = GetColor(0);
    info.m_trackId = m_data.m_id;
    info.m_trackPoint = position->GetPoint(*this);
    info.m_distFromBegM = position->GetDistanceMeters(*this);
  }
}

df::DepthLayer Track::GetDepthLayer() const
{
  return m_isSelected ? df::DepthLayer::SelectedUserLineLayer : df::DepthLayer::UserLineLayer;
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

bool Track::IsVisible() const
{
  return m_data.m_visible;
}

void Track::SetVisibility(bool visible)
{
  m_isDirty = true;
  m_data.m_visible = visible;
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
  m_isDirty = true;
  m_groupID = groupId;
}

void Track::Detach()
{
  m_isDirty = true;
  m_groupID = kml::kInvalidMarkGroupId;
}

m2::PointD Track::GetPoint(double distanceInMeters) const
{
  auto const & lengths = GetCumulativeLengths();
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
    return mercator::Interpolate(p1, p2, f);
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

std::pair<TrackStatistics, ElevationInfo const *> Track::GetStatisticsAndElevationInfo(
    ElevationInfo::Section const * elevationSection) const
{
  TrackStatistics ts;
  ts.m_length = GetLengthMeters();
  ts.CalculateDuration(m_data.m_geometry);

  auto const * ei = GetElevationInfo();
  if (ei)
  {
    // Relation tracks from MWM have cleaner altitude data than raw GPS tracks.
    auto const threshold =
        (m_data.m_id == kml::kTempRelationTrackId) ? ElevationInfo::kDefThresholdMWM : ElevationInfo::kDefThresholdGPS;
    auto const altInfo = ei->CalculateAltitudesInfo(threshold, elevationSection);
    ts.m_ascent = altInfo.GetTotalAscent();
    ts.m_descent = altInfo.GetTotalDescent();
    ts.m_minElevation = altInfo.m_minAltitude;
    ts.m_maxElevation = altInfo.m_maxAltitude;
  }
  return {ts, ei};
}

ElevationInfo const * Track::GetElevationInfo() const
{
  if (!m_elevationInfo)
  {
    if (!HasAltitudes())
      return nullptr;
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
