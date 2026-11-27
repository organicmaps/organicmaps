#include "map/gps_track_filter.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "platform/settings.hpp"

#include "base/assert.hpp"

namespace
{
std::string_view constexpr kMinHorizontalAccuracyKey = "GpsTrackingMinAccuracy";

// Minimal horizontal accuracy is required to skip 'bad' points.
// Use 250 meters to allow points from a pure GPS + GPS through wifi.
double constexpr kMinHorizontalAccuracyMeters = 250;

// Required for points decimation to reduce number of close points.
double constexpr kClosePointDistanceMeters = 10;

// Max acceptable acceleration to filter gps jumps
double constexpr kMaxAcceptableAcceleration = 2;  // m / sec ^ 2

double constexpr kCosine45degrees = 0.70710678118;

m2::PointD GetDirection(location::GpsInfo const & from, location::GpsInfo const & to)
{
  m2::PointD const pt0 = mercator::FromLatLon(from.m_latitude, from.m_longitude);
  m2::PointD const pt1 = mercator::FromLatLon(to.m_latitude, to.m_longitude);
  m2::PointD const d = pt1 - pt0;
  return d.IsAlmostZero() ? m2::PointD::Zero() : d.Normalize();
}

double GetDistance(location::GpsInfo const & from, location::GpsInfo const & to)
{
  return ms::DistanceOnEarth(from.m_latitude, from.m_longitude, to.m_latitude, to.m_longitude);
}
}  // namespace

void GpsTrackNullFilter::Process(std::vector<location::GpsInfo> const & inPoints,
                                 std::vector<location::GpsInfo> & outPoints)
{
  outPoints.insert(outPoints.end(), inPoints.begin(), inPoints.end());
}

void GpsTrackFilter::StoreMinHorizontalAccuracy(double value)
{
  settings::Set(kMinHorizontalAccuracyKey, value);
}

GpsTrackFilter::GpsTrackFilter() : m_minAccuracy(kMinHorizontalAccuracyMeters)
{
  settings::TryGet(kMinHorizontalAccuracyKey, m_minAccuracy);
}

void GpsTrackFilter::Reset()
{
  m_countLastInfo = 0;
}

void GpsTrackFilter::Process(GpsVectorT const & inPoints, GpsVectorT & outPoints)
{
  outPoints.reserve(inPoints.size());

  if (m_minAccuracy == 0)
  {
    // Debugging trick to turn off filtering
    outPoints.insert(outPoints.end(), inPoints.begin(), inPoints.end());
    return;
  }

  for (location::GpsInfo const & currInfo : inPoints)
  {
    // Jump filtering needs the previous fix's speed to estimate acceleration.
    if (!currInfo.HasSpeed())
      continue;

    // Exclude inaccurate fixes from the jump and direction checks and from the final track endpoint.
    if (currInfo.m_horizontalAccuracy > m_minAccuracy)
      continue;

    // A replayed fix (same time and position) must not enter the history: IsGoodVector() would take a zero
    // direction from it and reject the next fix. Other fixes still do, also an older one, so the filter follows
    // a backward clock correction.
    if (m_countLastInfo > 0)
    {
      auto const & lastInfo = GetLastInfo();
      if (currInfo.m_timestamp == lastInfo.m_timestamp && currInfo.GetLatLon() == lastInfo.GetLatLon())
        continue;
    }

    if (m_countLastInfo == 0 || (IsGoodPoint(currInfo) && (m_countLastInfo < 2 || IsGoodVector(currInfo))))
    {
      outPoints.emplace_back(currInfo);
      m_lastAcceptedInfo = currInfo;
    }

    AddLastInfo(currInfo);
  }
}

void GpsTrackFilter::Finalize(GpsVectorT & outPoints)
{
  if (m_countLastInfo > 0)
  {
    // Offer the final fix even if sampling skipped it. The collection filters duplicate positions.
    auto const & info = GetLastInfo();
    auto const & lastAccepted = GetLastAcceptedInfo();
    if (info.m_timestamp > lastAccepted.m_timestamp)
    {
      outPoints.push_back(info);
      m_lastAcceptedInfo = info;
    }
  }
}

bool GpsTrackFilter::IsGoodVector(location::GpsInfo const & info) const
{
  ASSERT_GREATER(m_countLastInfo, 1, ());

  double const predictionDistance = GetDistance(m_lastInfo[1], m_lastInfo[0]);  // meters
  double const realDistance = GetDistance(m_lastInfo[0], info);                 // meters

  m2::PointD const predictionDirection = GetDirection(m_lastInfo[1], m_lastInfo[0]);
  m2::PointD const realDirection = GetDirection(m_lastInfo[0], info);

  // Cosine of angle between prediction direction and real direction is
  double const cosine = m2::DotProduct(predictionDirection, realDirection);

  // Acceptable angle must be from 0 to 45 or from 0 to -45.
  // Acceptable distance must be not great than 2x than predicted, otherwise it is jump.
  return (cosine >= kCosine45degrees && realDistance <= std::max(kClosePointDistanceMeters, 2. * predictionDistance));
}

bool GpsTrackFilter::IsGoodPoint(location::GpsInfo const & info) const
{
  auto const & lastInfo = GetLastInfo();
  auto const & lastAcceptedInfo = GetLastAcceptedInfo();

  // Time spend to move from the last point to the current point, sec:
  double const timeFromLast = info.m_timestamp - lastInfo.m_timestamp;
  if (timeFromLast <= 0.0)
    return false;

  // Distance in meters between last accepted and current point is, meters:
  double const distanceFromLastAccepted = GetDistance(lastAcceptedInfo, info);

  // Filter point by close distance
  if (distanceFromLastAccepted < kClosePointDistanceMeters)
    return false;

  // Filter point if accuracy areas are intersected
  if (distanceFromLastAccepted < lastAcceptedInfo.m_horizontalAccuracy &&
      info.m_horizontalAccuracy > 0.5 * lastAcceptedInfo.m_horizontalAccuracy)
    return false;

  // Distance in meters between last and current point is, meters:
  double const distanceFromLast = GetDistance(lastInfo, info);

  // Speed to move from the last point to the current point
  double const speedFromLast = distanceFromLast / timeFromLast;

  // Filter by acceleration: skip point if it jumps too far in short time
  double const accelerationFromLast = (speedFromLast - lastInfo.m_speed) / timeFromLast;
  if (accelerationFromLast > kMaxAcceptableAcceleration)
    return false;

  return true;
}

location::GpsInfo const & GpsTrackFilter::GetLastInfo() const
{
  ASSERT_GREATER(m_countLastInfo, 0, ());
  return m_lastInfo[0];
}

location::GpsInfo const & GpsTrackFilter::GetLastAcceptedInfo() const
{
  ASSERT_GREATER(m_countLastInfo, 0, ());
  return m_lastAcceptedInfo;
}

void GpsTrackFilter::AddLastInfo(location::GpsInfo const & info)
{
  m_lastInfo[1] = m_lastInfo[0];
  m_lastInfo[0] = info;
  ++m_countLastInfo;
}
