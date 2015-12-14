#include "map/gps_track_filter.hpp"

#include "platform/settings.hpp"

namespace
{

double constexpr kMinHorizontalAccuracyMeters = 50;

char const kMinHorizontalAccuracyKey[] = "MinHorizontalAccuracy";

} // namespace

void GpsTrackFilter::StoreMinHorizontalAccuracy(double value)
{
  Settings::Set(kMinHorizontalAccuracyKey, value);
}

GpsTrackFilter::GpsTrackFilter()
  : m_minAccuracy(kMinHorizontalAccuracyMeters)
{
  Settings::Get(kMinHorizontalAccuracyKey, m_minAccuracy);
}

void GpsTrackFilter::Process(vector<location::GpsInfo> const & inPoints,
                             vector<location::GpsTrackInfo> & outPoints)
{
  // Very simple initial implementation of filter.
  // Further, it is going to be improved.

  outPoints.reserve(inPoints.size());

  for (auto const & inPt : inPoints)
  {
    if (m_minAccuracy > 0 && inPt.m_horizontalAccuracy > m_minAccuracy)
      continue;

    outPoints.emplace_back(inPt);
  }
}
