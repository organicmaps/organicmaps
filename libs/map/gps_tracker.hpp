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

  TrackStatistics GetTrackStatistics();
  ElevationInfo const & GetElevationInfo();

  using TGpsTrackDiffCallback = GpsTrack::TGpsTrackDiffCallback;

  void Connect(TGpsTrackDiffCallback const & fn);
  void Disconnect();

  void SetStatisticsCallback(GpsTrack::TGpsTrackStatisticsCallback const & fn);

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
