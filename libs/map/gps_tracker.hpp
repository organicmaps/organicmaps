#pragma once

#include "map/gps_track.hpp"

#include <atomic>
#include <functional>
#include <vector>

class GpsTracker
{
public:
  static GpsTracker & Instance();

  bool IsEnabled() const;
  void SetEnabled(bool enabled);
  void Clear();

  bool IsEmpty() const;

  TrackStatistics GetTrackStatistics() const;
  ElevationInfo const & GetElevationInfo() const;

  using TGpsTrackDiffCallback =
      std::function<void(std::vector<std::pair<size_t, location::GpsInfo>> && toAdd,
                         std::pair<size_t, size_t> const & toRemove, TrackStatistics const & trackStatistics)>;

  void Connect(TGpsTrackDiffCallback const & fn);
  void Disconnect();

  void OnLocationUpdated(location::GpsInfo const & info);

  size_t Finalize() { return m_track.Finalize(); }

  /// @pre Finalize should be called before.
  template <class FnT>
  void ForEachTrackPoint(FnT && fn)
  {
    m_track.ForEachPoint(fn);
  }

private:
  GpsTracker();

  std::atomic<bool> m_enabled;
  GpsTrack m_track;
};
