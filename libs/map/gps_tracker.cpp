#include "map/gps_tracker.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "base/file_name_utils.hpp"

#include "defines.hpp"

namespace
{

std::string_view constexpr kEnabledKey = "GpsTrackingEnabled";

inline std::string GetFilePath()
{
  return base::JoinPath(GetPlatform().WritableDir(), GPS_TRACK_FILENAME);
}

inline bool GetSettingsIsEnabled()
{
  bool enabled;
  if (!settings::Get(kEnabledKey, enabled))
    enabled = false;
  return enabled;
}

inline void SetSettingsIsEnabled(bool enabled)
{
  settings::Set(kEnabledKey, enabled);
}

}  // namespace

GpsTracker & GpsTracker::Instance()
{
  static GpsTracker instance;
  return instance;
}

GpsTracker::GpsTracker() : m_enabled(GetSettingsIsEnabled()), m_track(GetFilePath(), std::make_unique<GpsTrackFilter>())
{
  if (m_enabled)
    m_track.Start();
}

void GpsTracker::SetEnabled(bool enabled)
{
  if (enabled == m_enabled)
    return;

  SetSettingsIsEnabled(enabled);
  m_enabled = enabled;

  if (enabled)
    m_track.Start();
  else
    m_track.Finalize();
}

void GpsTracker::Clear()
{
  m_track.Clear();
}

bool GpsTracker::IsEnabled() const
{
  return m_enabled;
}

bool GpsTracker::IsEmpty() const
{
  return m_track.IsEmpty();
}

TrackStatistics GpsTracker::GetTrackStatistics()
{
  return m_track.GetTrackStatistics();
}

std::shared_ptr<GpsTrack::ElevationSnapshot const> GpsTracker::GetElevationSnapshot(
    GpsTrack::IsCancelled const & isCancelled)
{
  return m_track.GetElevationSnapshot(isCancelled);
}

void GpsTracker::Connect(TGpsTrackDiffCallback const & fn)
{
  m_track.SetCallback(fn);
}

void GpsTracker::Disconnect()
{
  m_track.SetCallback(nullptr);
}

void GpsTracker::SetStatisticsCallback(GpsTrack::TGpsTrackStatisticsCallback const & fn)
{
  m_track.SetStatisticsCallback(fn);
}

void GpsTracker::OnLocationUpdated(location::GpsInfo const & info)
{
  if (!m_enabled)
    return;
  m_track.AddPoint(info);
}
