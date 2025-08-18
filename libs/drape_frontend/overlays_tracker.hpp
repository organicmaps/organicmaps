#pragma once

#include "indexer/feature_decl.hpp"

#include <chrono>
#include <functional>
#include <list>
#include <map>
#include <vector>

namespace df
{
using EventClock = std::chrono::system_clock;
using EventTimestamp = EventClock::time_point;

struct OverlayShowEvent
{
  FeatureID m_feature;
  uint8_t m_zoomLevel;
  EventTimestamp m_timestamp;
  bool m_hasMyPosition;
  m2::PointD m_myPosition;
  double m_gpsAccuracy;
  OverlayShowEvent(FeatureID const & feature, uint8_t zoomLevel, EventTimestamp const & timestamp, bool hasMyPosition,
                   m2::PointD const & myPosition, double gpsAccuracy)
    : m_feature(feature)
    , m_zoomLevel(zoomLevel)
    , m_timestamp(timestamp)
    , m_hasMyPosition(hasMyPosition)
    , m_myPosition(myPosition)
    , m_gpsAccuracy(gpsAccuracy)
  {}
};

using OverlaysShowStatsCallback = std::function<void(std::list<OverlayShowEvent> &&)>;

int constexpr kMinZoomLevel = 10;

class OverlaysTracker
{
public:
  OverlaysTracker() = default;

  void SetTrackedOverlaysFeatures(std::vector<FeatureID> && ids);

  bool StartTracking(int zoomLevel, bool hasMyPosition, m2::PointD const & myPosition, double gpsAccuracy);
  void Track(FeatureID const & fid);
  void FinishTracking();

  std::list<OverlayShowEvent> Collect();

  bool IsValid() const { return !m_data.empty(); }

private:
  enum class OverlayStatus
  {
    Invisible,
    InvisibleCandidate,
    Visible,
  };

  struct OverlayInfo
  {
    EventTimestamp m_timestamp;
    OverlayStatus m_status = OverlayStatus::Invisible;
    bool m_tracked = false;
  };

  std::map<FeatureID, OverlayInfo> m_data;
  std::list<OverlayShowEvent> m_events;
  int m_zoomLevel = -1;
  bool m_hasMyPosition = false;
  m2::PointD m_myPosition = m2::PointD::Zero();
  double m_gpsAccuracy = 0.0;
};
}  // namespace df
