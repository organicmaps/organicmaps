#include "map/gps_track_filter.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "platform/settings.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/math.hpp"

namespace
{

char const kMinHorizontalAccuracyKey[] = "GpsTrackingMinAccuracy";

// Minimal horizontal accuracy is required to skip 'bad' points.
// Use 250 meters to allow points from a pure GPS + GPS through wifi.
double constexpr kMinHorizontalAccuracyMeters = 250;

// Required for points decimation to reduce number of close points.
double constexpr kClosePointDistanceMeters = 15;

// Max acceptable acceleration to filter gps jumps
double constexpr kMaxAcceptableAcceleration = 2; // m / sec ^ 2

double constexpr kCosine45degrees = 0.70710678118;

m2::PointD GetDirection(location::GpsInfo const & from, location::GpsInfo const & to)
{
  m2::PointD const pt0 = MercatorBounds::FromLatLon(from.m_latitude, from.m_longitude);
  m2::PointD const pt1 = MercatorBounds::FromLatLon(to.m_latitude, to.m_longitude);
  m2::PointD const d = pt1 - pt0;
  return d.IsAlmostZero() ? m2::PointD::Zero() : d.Normalize();
}

double GetDistance(location::GpsInfo const & from, location::GpsInfo const & to)
{
  return ms::DistanceOnEarth(from.m_latitude, from.m_longitude, to.m_latitude, to.m_longitude);
}

} // namespace

void GpsTrackNullFilter::Process(vector<location::GpsInfo> const & inPoints,
                                 vector<location::GpsTrackInfo> & outPoints)
{
  outPoints.insert(outPoints.end(), inPoints.begin(), inPoints.end());
}

void GpsTrackFilter::StoreMinHorizontalAccuracy(double value)
{
  settings::Set(kMinHorizontalAccuracyKey, value);
}

GpsTrackFilter::GpsTrackFilter()
  : m_minAccuracy(kMinHorizontalAccuracyMeters)
  , m_countLastInfo(0)
  , m_countAcceptedInfo(0)
{
  settings::TryGet(kMinHorizontalAccuracyKey, m_minAccuracy);
}

void GpsTrackFilter::Process(vector<location::GpsInfo> const & inPoints,
                             vector<location::GpsTrackInfo> & outPoints)
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
    // Do not accept points from the predictor
    if (currInfo.m_source == location::EPredictor)
      continue;

    // Skip any function without speed
    if (!currInfo.HasSpeed())
      continue;

    if (m_countAcceptedInfo < 2 || currInfo.m_timestamp < GetLastAcceptedInfo().m_timestamp)
    {
      AddLastInfo(currInfo);
      AddLastAcceptedInfo(currInfo);
      continue;
    }

    if (IsGoodPoint(currInfo))
    {
      double const predictionDistance = GetDistance(m_lastInfo[1], m_lastInfo[0]); // meters
      double const realDistance = GetDistance(m_lastInfo[0], currInfo); // meters

      m2::PointD const predictionDirection = GetDirection(m_lastInfo[1], m_lastInfo[0]);
      m2::PointD const realDirection = GetDirection(m_lastInfo[0], currInfo);

      // Cosine of angle between prediction direction and real direction is
      double const cosine = m2::DotProduct(predictionDirection, realDirection);

      // Acceptable angle must be from 0 to 45 or from 0 to -45.
      // Acceptable distance must be not great than 2x than predicted, otherwise it is jump.
      if (cosine >= kCosine45degrees &&
          realDistance <= max(kClosePointDistanceMeters, 2. * predictionDistance))
      {
        outPoints.emplace_back(currInfo);
        AddLastAcceptedInfo(currInfo);
      }
    }

    AddLastInfo(currInfo);
  }
}

bool GpsTrackFilter::IsGoodPoint(location::GpsInfo const & info) const
{
  // Filter by point accuracy
  if (info.m_horizontalAccuracy > m_minAccuracy)
    return false;

  auto const & lastInfo = GetLastInfo();
  auto const & lastAcceptedInfo = GetLastAcceptedInfo();

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

  // Time spend to move from the last point to the current point, sec:
  double const timeFromLast = info.m_timestamp - lastInfo.m_timestamp;
  if (timeFromLast <= 0.0)
    return false;

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
  ASSERT_GREATER(m_countAcceptedInfo, 0, ());
  return m_lastAcceptedInfo[0];
}

void GpsTrackFilter::AddLastInfo(location::GpsInfo const & info)
{
  m_lastInfo[1] = m_lastInfo[0];
  m_lastInfo[0] = info;
  m_countLastInfo += 1;
}

void GpsTrackFilter::AddLastAcceptedInfo(location::GpsInfo const & info)
{
  m_lastAcceptedInfo[1] = m_lastAcceptedInfo[0];
  m_lastAcceptedInfo[0] = info;
  m_countAcceptedInfo += 1;
}

