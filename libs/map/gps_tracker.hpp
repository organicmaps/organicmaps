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
  void Start();
  // Finalize the points while retaining the recording preference and backup.
  size_t Pause();
  void Resume();
  // Flush pending fixes and return the number of accepted points.
  size_t Stop();
  void Clear();

  TrackStatistics GetTrackStatistics();
  ElevationInfo const & GetElevationInfo();

  using TGpsTrackDiffCallback = GpsTrack::TGpsTrackDiffCallback;

  void Connect(TGpsTrackDiffCallback const & fn);
  void Disconnect();

  void SetStatisticsCallback(GpsTrack::TGpsTrackStatisticsCallback const & fn);

  void OnLocationUpdated(location::GpsInfo const & info);

  /// @pre Stop should be called before.
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
