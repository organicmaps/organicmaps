#include "map/gps_track_filter.hpp"

#include "geometry/mercator.hpp"

#include "platform/settings.hpp"

namespace
{

char const kMinHorizontalAccuracyKey[] = "GpsTrackingMinAccuracy";

// Minimal horizontal accuracy is required to skip 'bad' points.
double constexpr kMinHorizontalAccuracyMeters = 50;

// Required for points decimation to reduce number of close points.
double constexpr kClosePointDistanceMeters = 15;

} // namespace

void GpsTrackFilter::StoreMinHorizontalAccuracy(double value)
{
  Settings::Set(kMinHorizontalAccuracyKey, value);
}

GpsTrackFilter::GpsTrackFilter()
  : m_minAccuracy(kMinHorizontalAccuracyMeters)
  , m_lastPt(0, 0)
  , m_hasLast(false)
{
  Settings::Get(kMinHorizontalAccuracyKey, m_minAccuracy);
}

void GpsTrackFilter::Process(vector<location::GpsInfo> const & inPoints,
                             vector<location::GpsTrackInfo> & outPoints)
{
  // Very simple initial implementation of filter.
  // Further, it is going to be improved.

  outPoints.reserve(inPoints.size());

  for (auto const & originPt : inPoints)
  {
    // Filter point by accuracy
    if (m_minAccuracy > 0 && originPt.m_horizontalAccuracy > m_minAccuracy)
      continue;

    // Filter point by close distance
    m2::PointD const & pt = MercatorBounds::FromLatLon(originPt.m_latitude, originPt.m_longitude);
    if (m_hasLast && MercatorBounds::DistanceOnEarth(pt, m_lastPt) < kClosePointDistanceMeters)
      continue;

    m_lastPt = pt;
    m_hasLast = true;

    outPoints.emplace_back(originPt);
  }
}
