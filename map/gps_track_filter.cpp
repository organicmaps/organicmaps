#include "map/gps_track_filter.hpp"

namespace
{

double constexpr kMinHorizontalAccuracyMeters = 30;

} // namespace

void GpsTrackFilter::Process(vector<location::GpsInfo> const & inPoints,
                             vector<location::GpsTrackInfo> & outPoints)
{
  // Very simple initial implementation of filter.
  // Further, it is going to be improved.

  outPoints.reserve(inPoints.size());

  for (auto const & inPt : inPoints)
  {
    if (inPt.m_horizontalAccuracy > kMinHorizontalAccuracyMeters)
      continue;

    outPoints.emplace_back(inPt);
  }
}
