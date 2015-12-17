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

// Periodicy to check points in wifi afea, sec
size_t const kWifiAreaGpsPeriodicyCheckSec = 30;

// Max acceptable moving speed in wifi area, required to prevent jumping between wifi, m/s
double const kWifiAreaAcceptableMovingSpeedMps = 3;

inline bool IsRealGpsPoint(location::GpsInfo const & info)
{
  // we guess real gps says speed and bearing other than zero
  return info.m_speed > 0 && info.m_bearing > 0;
}

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
  steady_clock::time_point const timeNow = steady_clock::now();

  outPoints.reserve(inPoints.size());

  for (location::GpsInfo const & currInfo : inPoints)
  {
    if (!m_hasLastInfo)
    {
      // Accept first point
      m_hasLastInfo = true;
      m_lastInfo = currInfo;
      m_lastGoodGpsTime = timeNow;
      outPoints.emplace_back(currInfo);
      continue;
    }

    // Distance in meters between last and current point is, meters:
    double const distance = ms::DistanceOnEarth(m_lastInfo.m_latitude, m_lastInfo.m_longitude,
                                                currInfo.m_latitude, currInfo.m_longitude);

    // Filter point by close distance
    if (distance < kClosePointDistanceMeters)
      continue;

    // Filter point if accuracy areas are intersected
    if (distance < m_lastInfo.m_horizontalAccuracy && distance < currInfo.m_horizontalAccuracy)
      continue;

    // Filter by point accuracy
    if (currInfo.m_horizontalAccuracy > m_minAccuracy)
      continue;

    bool const lastRealGps = IsRealGpsPoint(m_lastInfo);
    bool const currRealGps = IsRealGpsPoint(currInfo);

    bool const gpsToWifi = lastRealGps && !currRealGps;
    bool const wifiToWifi = !lastRealGps && !currRealGps;

    if (gpsToWifi || wifiToWifi)
    {
      auto const elapsedTimeSinceGoodGps = duration_cast<seconds>(timeNow - m_lastGoodGpsTime);

      // Wait before switch gps to wifi or switch between wifi points
      if (elapsedTimeSinceGoodGps.count() < kWifiAreaGpsPeriodicyCheckSec)
        continue;

      // Skip point if moving to it was too fast, we guess it was a jump from wifi to another wifi
      double const speed = distance / elapsedTimeSinceGoodGps.count();
      if (speed > kWifiAreaAcceptableMovingSpeedMps)
        continue;
    }

    m_lastGoodGpsTime = timeNow;
    m_lastInfo = currInfo;
    outPoints.emplace_back(currInfo);
  }
}
