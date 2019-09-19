#pragma once

#include "map/gps_track.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <utility>
#include <vector>

class GpsTracker
{
public:
  static GpsTracker & Instance();

  bool IsEnabled() const;
  void SetEnabled(bool enabled);

  std::chrono::hours GetDuration() const;
  void SetDuration(std::chrono::hours duration);

  using TGpsTrackDiffCallback =
      std::function<void(std::vector<std::pair<size_t, location::GpsTrackInfo>> && toAdd,
                         std::pair<size_t, size_t> const & toRemove)>;

  void Connect(TGpsTrackDiffCallback const & fn);
  void Disconnect();

  void OnLocationUpdated(location::GpsInfo const & info);

private:
  GpsTracker();

  std::atomic<bool> m_enabled;
  GpsTrack m_track;
};
