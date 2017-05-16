#pragma once

#include "routing/num_mwm_id.hpp"
#include "routing/segment.hpp"

#include "coding/traffic.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace tracking
{
using DataPoint = coding::TrafficGPSEncoder::DataPoint;
using Track = std::vector<DataPoint>;
using UserToTrack = std::unordered_map<std::string, Track>;
using MwmToTracks = std::unordered_map<routing::NumMwmId, UserToTrack>;

class MatchedTrackPoint final
{
public:
  MatchedTrackPoint(DataPoint const & dataPoint, routing::Segment const & segment)
    : m_dataPoint(dataPoint), m_segment(segment)
  {
  }

  DataPoint const & GetDataPoint() const { return m_dataPoint; }
  routing::Segment const & GetSegment() const { return m_segment; }

private:
  DataPoint m_dataPoint;
  routing::Segment m_segment;
};

using MatchedTrack = std::vector<MatchedTrackPoint>;
using UserToMatchedTracks = std::unordered_map<std::string, std::vector<MatchedTrack>>;
using MwmToMatchedTracks = std::unordered_map<routing::NumMwmId, UserToMatchedTracks>;

class TrackFilter final
{
public:
  TrackFilter(uint64_t minDuration, double minLength, double minSpeed, double maxSpeed,
              bool ignoreTraffic)
    : m_minDuration(minDuration)
    , m_minLength(minLength)
    , m_minSpeed(minSpeed)
    , m_maxSpeed(maxSpeed)
    , m_ignoreTraffic(ignoreTraffic)
  {
  }

  bool Passes(uint64_t duration, double length, double speed, bool hasTrafficPoints) const
  {
    if (duration < m_minDuration)
      return false;

    if (length < m_minLength)
      return false;

    if (speed < m_minSpeed || speed > m_maxSpeed)
      return false;

    return !(m_ignoreTraffic && hasTrafficPoints);
  }

private:
  uint64_t const m_minDuration;
  double const m_minLength;
  double const m_minSpeed;
  double const m_maxSpeed;
  bool m_ignoreTraffic;
};
}  // namespace tracking
