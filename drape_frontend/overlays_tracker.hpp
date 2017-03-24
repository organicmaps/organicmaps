#pragma once

#include "indexer/feature_decl.hpp"

#include <chrono>
#include <functional>
#include <list>
#include <map>
#include <vector>

namespace df
{
struct OverlayShowEvent
{
  FeatureID m_feature;
  int m_zoomLevel;
  std::chrono::system_clock::time_point m_timestamp;
  OverlayShowEvent(FeatureID const & feature, int zoomLevel,
                   std::chrono::system_clock::time_point const & timestamp)
    : m_feature(feature)
    , m_zoomLevel(zoomLevel)
    , m_timestamp(timestamp)
  {}
};

using OverlaysShowStatsCallback = std::function<void(std::list<OverlayShowEvent> &&)>;

int constexpr kMinZoomLevel = 10;

class OverlaysTracker
{
public:
  OverlaysTracker() = default;

  void SetTrackedOverlaysFeatures(std::vector<FeatureID> && ids);

  bool StartTracking(int zoomLevel);
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
    std::chrono::steady_clock::time_point m_timestamp;
    OverlayStatus m_status = OverlayStatus::Invisible;
    bool m_tracked = false;
  };

  std::map<FeatureID, OverlayInfo> m_data;
  std::list<OverlayShowEvent> m_events;
  int m_zoomLevel = -1;
};

}  // namespace df
