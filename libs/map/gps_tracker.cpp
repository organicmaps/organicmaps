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

void GpsTracker::Start()
{
  if (m_enabled)
    return;

  SetSettingsIsEnabled(true);
  m_track.Start();
  m_enabled = true;
}

size_t GpsTracker::Stop()
{
  if (m_enabled.exchange(false))
    SetSettingsIsEnabled(false);
  return m_track.Finalize();
}

bool GpsTracker::CanSave()
{
  return m_track.CanSave();
}

void GpsTracker::Clear()
{
  m_track.Clear();
}

bool GpsTracker::IsEnabled() const
{
  return m_enabled;
}

TrackStatistics GpsTracker::GetTrackStatistics()
{
  return m_track.GetTrackStatistics();
}

ElevationInfo const & GpsTracker::GetElevationInfo()
{
  return m_track.GetElevationInfo();
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
