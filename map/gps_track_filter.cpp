#include "map/gps_track_filter.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "platform/settings.hpp"

#include "std/algorithm.hpp"

namespace
{

char const kMinHorizontalAccuracyKey[] = "GpsTrackingMinAccuracy";

// Minimal horizontal accuracy is required to skip 'bad' points.
double constexpr kMinHorizontalAccuracyMeters = 50;

// Required for points decimation to reduce number of close points.
double constexpr kClosePointDistanceMeters = 15;

} // namespace

void GpsTrackNullFilter::Process(vector<location::GpsInfo> const & inPoints,
                                 vector<location::GpsTrackInfo> & outPoints)
{
  outPoints.reserve(inPoints.size());
  copy(inPoints.begin(), inPoints.end(), back_inserter(outPoints));
}

void GpsTrackFilter::StoreMinHorizontalAccuracy(double value)
{
  Settings::Set(kMinHorizontalAccuracyKey, value);
}

GpsTrackFilter::GpsTrackFilter()
  : m_minAccuracy(kMinHorizontalAccuracyMeters)
  , m_lastLl(0, 0)
  , m_hasLastLl(false)
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
    ms::LatLon const ll(originPt.m_latitude, originPt.m_longitude);
    if (m_hasLastLl && ms::DistanceOnEarth(m_lastLl, ll) < kClosePointDistanceMeters)
      continue;

    m_lastLl = ll;
    m_hasLastLl = true;

    outPoints.emplace_back(originPt);
  }
}
