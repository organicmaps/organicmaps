#include "drape_frontend/overlays_tracker.hpp"

namespace df
{
void OverlaysTracker::SetTrackedOverlaysFeatures(std::vector<FeatureID> && ids)
{
  m_data.clear();
  for (auto const & fid : ids)
    m_data.insert(std::make_pair(fid, OverlayInfo()));
}

bool OverlaysTracker::StartTracking(int zoomLevel, bool hasMyPosition, m2::PointD const & myPosition,
                                    double gpsAccuracy)
{
  if (zoomLevel < kMinZoomLevel)
  {
    for (auto & p : m_data)
      p.second.m_status = OverlayStatus::Invisible;
    return false;
  }

  m_zoomLevel = zoomLevel;
  m_hasMyPosition = hasMyPosition;
  m_myPosition = m_hasMyPosition ? myPosition : m2::PointD();
  m_gpsAccuracy = m_hasMyPosition ? gpsAccuracy : 0.0;
  for (auto & p : m_data)
    p.second.m_tracked = false;

  return true;
}

void OverlaysTracker::Track(FeatureID const & fid)
{
  ASSERT_GREATER_OR_EQUAL(m_zoomLevel, kMinZoomLevel, ());

  auto it = m_data.find(fid);
  if (it == m_data.end())
    return;

  it->second.m_tracked = true;
  if (it->second.m_status == OverlayStatus::Invisible)
  {
    it->second.m_status = OverlayStatus::Visible;
    m_events.emplace_back(it->first, static_cast<uint8_t>(m_zoomLevel), EventClock::now(), m_hasMyPosition,
                          m_myPosition, m_gpsAccuracy);
  }
}

void OverlaysTracker::FinishTracking()
{
  ASSERT_GREATER_OR_EQUAL(m_zoomLevel, kMinZoomLevel, ());

  for (auto & p : m_data)
  {
    if (p.second.m_status == OverlayStatus::Visible && !p.second.m_tracked)
    {
      p.second.m_status = OverlayStatus::InvisibleCandidate;
      p.second.m_timestamp = EventClock::now();
    }
    else if (p.second.m_status == OverlayStatus::InvisibleCandidate)
    {
      // Here we add some delay to avoid false events appearance.
      static auto const kDelay = std::chrono::milliseconds(500);
      if (p.second.m_tracked)
        p.second.m_status = OverlayStatus::Visible;
      else if (EventClock::now() - p.second.m_timestamp > kDelay)
        p.second.m_status = OverlayStatus::Invisible;
    }
  }
  m_zoomLevel = -1;
}

std::list<OverlayShowEvent> OverlaysTracker::Collect()
{
  std::list<OverlayShowEvent> events;
  std::swap(m_events, events);
  return events;
}
}  // namespace df
