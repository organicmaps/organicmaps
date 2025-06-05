#pragma once

#include "traffic/speed_groups.hpp"

#include "indexer/mwm_set.hpp"

#include <cstdint>
#include <map>
#include <vector>

namespace platform
{
class HttpClient;
}

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

    bool operator==(RoadSegmentId const & o) const { return m_fid == o.m_fid && m_idx == o.m_idx && m_dir == o.m_dir; }

    bool operator<(RoadSegmentId const & o) const
    {
      if (m_fid != o.m_fid)
        return m_fid < o.m_fid;
      if (m_idx != o.m_idx)
        return m_idx < o.m_idx;
      return m_dir < o.m_dir;
    }

    uint32_t GetFid() const { return m_fid; }
    uint16_t GetIdx() const { return m_idx; }
    uint8_t GetDir() const { return m_dir; }

    // The ordinal number of feature this segment belongs to.
    uint32_t m_fid;

    // The ordinal number of this segment in the list of
    // its feature's segments.
    uint16_t m_idx : 15;

    // The direction of the segment.
    uint8_t m_dir : 1;
  };

  // todo(@m) unordered_map?
  using Coloring = std::map<RoadSegmentId, SpeedGroup>;

  TrafficInfo() = default;

  TrafficInfo(MwmSet::MwmId const & mwmId, int64_t currentDataVersion);

  static TrafficInfo BuildForTesting(Coloring && coloring);
  void SetTrafficKeysForTesting(std::vector<RoadSegmentId> const & keys);

  // Fetches the latest traffic data from the server and updates the coloring and ETag.
  // Construct the url by passing an MwmId.
  // The ETag or entity tag is part of HTTP, the protocol for the World Wide Web.
  // It is one of several mechanisms that HTTP provides for web cache validation,
  // which allows a client to make conditional requests.
  // *NOTE* This method must not be called on the UI thread.
  bool ReceiveTrafficData(std::string & etag);

  // Returns the latest known speed group by a feature segment's id
  // or SpeedGroup::Unknown if there is no information about the segment.
  SpeedGroup GetSpeedGroup(RoadSegmentId const & id) const;

  MwmSet::MwmId const & GetMwmId() const { return m_mwmId; }
  Coloring const & GetColoring() const { return m_coloring; }
  Availability GetAvailability() const { return m_availability; }

  // Extracts RoadSegmentIds from mwm and stores them in a sorted order.
  static void ExtractTrafficKeys(std::string const & mwmPath, std::vector<RoadSegmentId> & result);

  // Adds the unknown values to the partially known coloring map |knownColors|
  // so that the keys of the resulting map are exactly |keys|.
  static void CombineColorings(std::vector<TrafficInfo::RoadSegmentId> const & keys,
                               TrafficInfo::Coloring const & knownColors, TrafficInfo::Coloring & result);

  // Serializes the keys of the coloring map to |result|.
  // The keys are road segments ids which do not change during
  // an mwm's lifetime so there's no point in downloading them every time.
  // todo(@m) Document the format.
  static void SerializeTrafficKeys(std::vector<RoadSegmentId> const & keys, std::vector<uint8_t> & result);

  static void DeserializeTrafficKeys(std::vector<uint8_t> const & data, std::vector<RoadSegmentId> & result);

  static void SerializeTrafficValues(std::vector<SpeedGroup> const & values, std::vector<uint8_t> & result);

  static void DeserializeTrafficValues(std::vector<uint8_t> const & data, std::vector<SpeedGroup> & result);

private:
  enum class ServerDataStatus
  {
    New,
    NotChanged,
    NotFound,
    Error,
  };

  friend void UnitTest_TrafficInfo_UpdateTrafficData();

  // todo(@m) A temporary method. Remove it once the keys are added
  // to the generator and the data is regenerated.
  bool ReceiveTrafficKeys();

  // Tries to read the values of the Coloring map from server into |values|.
  // Returns result of communicating with server as ServerDataStatus.
  // Otherwise, returns false and does not change m_coloring.
  ServerDataStatus ReceiveTrafficValues(std::string & etag, std::vector<SpeedGroup> & values);

  // Updates the coloring and changes the availability status if needed.
  bool UpdateTrafficData(std::vector<SpeedGroup> const & values);

  ServerDataStatus ProcessFailure(platform::HttpClient const & request, int64_t const mwmVersion);

  // The mapping from feature segments to speed groups (see speed_groups.hpp).
  Coloring m_coloring;

  // The keys of the coloring map. The values are downloaded periodically
  // and combined with the keys to form m_coloring.
  // *NOTE* The values must be received in the exact same order that the
  // keys are saved in.
  std::vector<RoadSegmentId> m_keys;

  MwmSet::MwmId m_mwmId;
  Availability m_availability = Availability::Unknown;
  int64_t m_currentDataVersion = 0;
};

class TrafficObserver
{
public:
  virtual ~TrafficObserver() = default;

  virtual void OnTrafficInfoClear() = 0;
  virtual void OnTrafficInfoAdded(traffic::TrafficInfo && info) = 0;
  virtual void OnTrafficInfoRemoved(MwmSet::MwmId const & mwmId) = 0;
};

std::string DebugPrint(TrafficInfo::RoadSegmentId const & id);
}  // namespace traffic
