#pragma once

#include "traffic/speed_groups.hpp"

#include "indexer/mwm_set.hpp"

#include "std/cstdint.hpp"
#include "std/map.hpp"
#include "std/shared_ptr.hpp"
#include "std/vector.hpp"

namespace traffic
{
// This class is responsible for providing the real-time
// information about road traffic for one mwm file.

class TrafficInfo
{
public:
  enum class Availability
  {
    IsAvailable,
    NoData,
    ExpiredData,
    ExpiredApp,
    Unknown
  };

  struct RoadSegmentId
  {
    // m_dir can be kForwardDirection or kReverseDirection.
    static int constexpr kForwardDirection = 0;
    static int constexpr kReverseDirection = 1;

    RoadSegmentId();

    RoadSegmentId(uint32_t fid, uint16_t idx, uint8_t dir);

    bool operator==(RoadSegmentId const & o) const
    {
      return m_fid == o.m_fid && m_idx == o.m_idx && m_dir == o.m_dir;
    }

    bool operator<(RoadSegmentId const & o) const
    {
      if (m_fid != o.m_fid)
        return m_fid < o.m_fid;
      if (m_idx != o.m_idx)
        return m_idx < o.m_idx;
      return m_dir < o.m_dir;
    }

    // The ordinal number of feature this segment belongs to.
    uint32_t m_fid;

    // The ordinal number of this segment in the list of
    // its feature's segments.
    uint16_t m_idx : 15;

    // The direction of the segment.
    uint8_t m_dir : 1;
  };

  // todo(@m) unordered_map?
  using Coloring = map<RoadSegmentId, SpeedGroup>;

  TrafficInfo() = default;

  TrafficInfo(MwmSet::MwmId const & mwmId, int64_t currentDataVersion);

  TrafficInfo(TrafficInfo && info) : m_coloring(move(info.m_coloring)), m_mwmId(info.m_mwmId) {}

  void SetColoringForTesting(Coloring & coloring) { m_coloring = coloring; }
  // Fetches the latest traffic data from the server and updates the coloring.
  // Construct the url by passing an MwmId.
  // *NOTE* This method must not be called on the UI thread.
  bool ReceiveTrafficData();

  // Returns the latest known speed group by a feature segment's id.
  SpeedGroup GetSpeedGroup(RoadSegmentId const & id) const;

  MwmSet::MwmId const & GetMwmId() const { return m_mwmId; }
  Coloring const & GetColoring() const { return m_coloring; }
  Availability GetAvailability() const { return m_availability; }

  static void SerializeTrafficData(Coloring const & coloring, vector<uint8_t> & result);

  static void DeserializeTrafficData(vector<uint8_t> const & data, Coloring & coloring);

private:
  // The mapping from feature segments to speed groups (see speed_groups.hpp).
  Coloring m_coloring;
  MwmSet::MwmId m_mwmId;
  Availability m_availability = Availability::Unknown;
  int64_t m_currentDataVersion = 0;
};

class TrafficObserver
{
public:
  virtual ~TrafficObserver() = default;

  virtual void OnTrafficEnabled(bool enable) = 0;
  virtual void OnTrafficInfoAdded(traffic::TrafficInfo && info) = 0;
  virtual void OnTrafficInfoRemoved(MwmSet::MwmId const & mwmId) = 0;
};

class TrafficInfoGetter
{
public:
  virtual ~TrafficInfoGetter() = default;

  virtual shared_ptr<traffic::TrafficInfo> GetTrafficInfo(MwmSet::MwmId const & mwmId) const = 0;
};
}  // namespace traffic
