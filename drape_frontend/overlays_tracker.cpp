#include "drape_frontend/overlays_tracker.hpp"

namespace df
{
void OverlaysTracker::SetTrackedOverlaysFeatures(std::vector<FeatureID> && ids)
{
  m_data.clear();
  for (auto const & fid : ids)
    m_data.insert(std::make_pair(fid, OverlayInfo()));
}

bool OverlaysTracker::StartTracking(int zoomLevel)
{
  if (zoomLevel < kMinZoomLevel)
  {
    for (auto & p : m_data)
      p.second.m_status = OverlayStatus::Invisible;
    return false;
  }

  m_zoomLevel = zoomLevel;
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
    m_events.emplace_back(it->first, m_zoomLevel, std::chrono::system_clock::now());
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
      p.second.m_timestamp = std::chrono::steady_clock::now();
    }
    else if (p.second.m_status == OverlayStatus::InvisibleCandidate)
    {
      // Here we add some delay to avoid false events appearance.
      static auto const kDelay = std::chrono::milliseconds(500);
      if (p.second.m_tracked)
        p.second.m_status = OverlayStatus::Visible;
      else if (std::chrono::steady_clock::now() - p.second.m_timestamp > kDelay)
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
