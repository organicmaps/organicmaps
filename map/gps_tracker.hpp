#pragma once

#include "map/gps_track.hpp"

#include "std/atomic.hpp"

class GpsTracker
{
public:
  static GpsTracker & Instance();

  bool IsEnabled() const;
  void SetEnabled(bool enabled);

  hours GetDuration() const;
  void SetDuration(hours duration);

  using TGpsTrackDiffCallback = std::function<void(vector<pair<size_t, location::GpsTrackInfo>> && toAdd,
                                                   pair<size_t, size_t> const & toRemove)>;

  void Connect(TGpsTrackDiffCallback const & fn);
  void Disconnect();

  void OnLocationUpdated(location::GpsInfo const & info);

private:
  GpsTracker();

  atomic<bool> m_enabled;
  GpsTrack m_track;
};
