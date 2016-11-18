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
  static uint8_t const kLatestKeysVersion;
  static uint8_t const kLatestValuesVersion;

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
    static uint8_t constexpr kForwardDirection = 0;
    static uint8_t constexpr kReverseDirection = 1;

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

  static TrafficInfo BuildForTesting(Coloring && coloring);

  // Fetches the latest traffic data from the server and updates the coloring.
  // Construct the url by passing an MwmId.
  // *NOTE* This method must not be called on the UI thread.
  bool ReceiveTrafficData();

  // Returns the latest known speed group by a feature segment's id
  // or SpeedGroup::Unknown if there is no information about the segment.
  SpeedGroup GetSpeedGroup(RoadSegmentId const & id) const;

  MwmSet::MwmId const & GetMwmId() const { return m_mwmId; }
  Coloring const & GetColoring() const { return m_coloring; }
  Availability GetAvailability() const { return m_availability; }

  // Extracts RoadSegmentIds from mwm and stores them in a sorted order.
  static void ExtractTrafficKeys(string const & mwmPath, vector<RoadSegmentId> & result);

  // Adds the unknown values to the partially known coloring map |knownColors|
  // so that the keys of the resulting map are exactly |keys|.
  static void CombineColorings(vector<TrafficInfo::RoadSegmentId> const & keys,
                               TrafficInfo::Coloring const & knownColors,
                               TrafficInfo::Coloring & result);

  // Serializes the keys of the coloring map to |result|.
  // The keys are road segments ids which do not change during
  // an mwm's lifetime so there's no point in downloading them every time.
  // todo(@m) Document the format.
  static void SerializeTrafficKeys(vector<RoadSegmentId> const & keys, vector<uint8_t> & result);

  static void DeserializeTrafficKeys(vector<uint8_t> const & data, vector<RoadSegmentId> & result);

  static void SerializeTrafficValues(vector<SpeedGroup> const & values, vector<uint8_t> & result);

  static void DeserializeTrafficValues(vector<uint8_t> const & data, vector<SpeedGroup> & result);

private:
  // todo(@m) A temporary method. Remove it once the keys are added
  // to the generator and the data is regenerated.
  bool ReceiveTrafficKeys();

  // Tries to read the values of the Coloring map from server.
  // Returns true and updates m_coloring if the values are read successfully and
  // their number is equal to the number of keys.
  // Otherwise, returns false and does not change m_coloring.
  bool ReceiveTrafficValues(vector<SpeedGroup> & values);

  // The mapping from feature segments to speed groups (see speed_groups.hpp).
  Coloring m_coloring;

  // The keys of the coloring map. The values are downloaded periodically
  // and combined with the keys to form m_coloring.
  // *NOTE* The values must be received in the exact same order that the
  // keys are saved in.
  vector<RoadSegmentId> m_keys;

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

string DebugPrint(TrafficInfo::RoadSegmentId const & id);
}  // namespace traffic
