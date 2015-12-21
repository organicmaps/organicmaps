#include "map/gps_track_filter.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "platform/settings.hpp"

namespace
{

char const kMinHorizontalAccuracyKey[] = "GpsTrackingMinAccuracy";

// Minimal horizontal accuracy is required to skip 'bad' points.
// Use 250 meters to allow points from a pure GPS + GPS through wifi.
double constexpr kMinHorizontalAccuracyMeters = 250;

// Required for points decimation to reduce number of close points.
double constexpr kClosePointDistanceMeters = 15;

// Max acceptable acceleration to filter gps jumps
double const kMaxAcceptableAcceleration = 2; // m / sec ^ 2

} // namespace

void GpsTrackNullFilter::Process(vector<location::GpsInfo> const & inPoints,
                                 vector<location::GpsTrackInfo> & outPoints)
{
  outPoints.insert(outPoints.end(), inPoints.begin(), inPoints.end());
}

void GpsTrackFilter::StoreMinHorizontalAccuracy(double value)
{
  Settings::Set(kMinHorizontalAccuracyKey, value);
}

GpsTrackFilter::GpsTrackFilter()
  : m_minAccuracy(kMinHorizontalAccuracyMeters)
  , m_hasLastInfo(false)
{
  Settings::Get(kMinHorizontalAccuracyKey, m_minAccuracy);
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

    if (!m_hasLastInfo || currInfo.m_timestamp < m_lastAcceptedInfo.m_timestamp)
    {
      m_hasLastInfo = true;
      m_lastAcceptedInfo = currInfo;
    }
    else if (IsGoodPoint(currInfo))
    {
      outPoints.emplace_back(currInfo);
    }

    m_lastInfo = currInfo;
  }
}

bool GpsTrackFilter::IsGoodPoint(location::GpsInfo const & info) const
{
  // Filter by point accuracy
  if (info.m_horizontalAccuracy > m_minAccuracy)
    return false;

  // Distance in meters between last accepted and current point is, meters:
  double const distanceFromLastAccepted = ms::DistanceOnEarth(m_lastAcceptedInfo.m_latitude, m_lastAcceptedInfo.m_longitude,
                                                              info.m_latitude, info.m_longitude);

  // Filter point by close distance
  if (distanceFromLastAccepted < kClosePointDistanceMeters)
    return false;

  // Filter point if accuracy areas are intersected
  if (distanceFromLastAccepted < m_lastAcceptedInfo.m_horizontalAccuracy &&
      info.m_horizontalAccuracy > 0.5 * m_lastAcceptedInfo.m_horizontalAccuracy)
    return false;

  // Distance in meters between last and current point is, meters:
  double const distanceFromLast = ms::DistanceOnEarth(m_lastInfo.m_latitude, m_lastInfo.m_longitude,
                                                      info.m_latitude, info.m_longitude);

  // Time spend to move from the last point to the current point, sec:
  double const timeFromLast = info.m_timestamp - m_lastInfo.m_timestamp;
  if (timeFromLast <= 0.0)
    return false;

  // Speed to move from the last point to the current point
  double const speedFromLast = distanceFromLast / timeFromLast;

  // Filter by acceleration: skip point it jumps too far in short time
  double const accelerationFromLast = (speedFromLast - m_lastInfo.m_speed) / timeFromLast;
  if (accelerationFromLast > kMaxAcceptableAcceleration)
    return false;

  return true;
}
