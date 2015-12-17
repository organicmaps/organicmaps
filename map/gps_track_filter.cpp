#include "map/gps_track_filter.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "platform/settings.hpp"

namespace
{

char const kMinHorizontalAccuracyKey[] = "GpsTrackingMinAccuracy";

// Minimal horizontal accuracy is required to skip 'bad' points.
double constexpr kMinHorizontalAccuracyMeters = 250;

// Required for points decimation to reduce number of close points.
double constexpr kClosePointDistanceMeters = 15;

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

  for (location::GpsInfo const & currInfo : inPoints)
  {
    if (!m_hasLastInfo)
    {
      m_hasLastInfo = true;
      m_lastInfo = currInfo;
      outPoints.emplace_back(currInfo);
      continue;
    }

    // Filter points which happens earlier than first point
    if (currInfo.m_timestamp <= m_lastInfo.m_timestamp)
      continue;

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

    // Time elapsed since last info is, sec:
    double const elapsedTime = currInfo.m_timestamp - m_lastInfo.m_speed;

    // Prevent jumping between wifi points
    // We use following heuristics: if suddenly point jumps over long
    // distance in short time then we skip point
    if (m_lastInfo.m_speed == 0 && currInfo.m_speed == 0)
    {
      if (distance > 25 && elapsedTime < 10)
        continue; // we guess it is jump between wifi points
    }

    m_lastInfo = currInfo;
    outPoints.emplace_back(currInfo);
  }
}
