#include "map/location_country_tracker.hpp"

#include "platform/platform.hpp"

void LocationCountryTracker::SetListener(TCountryChangedFn listener)
{
  m_listener = std::move(listener);
  m_currentCountry = storage::kInvalidCountryId;
  m_currentCountryRect = m2::RectD();
  m_lastCheckedPos = m2::PointD::Zero();
}

void LocationCountryTracker::OnLocationUpdate(m2::PointD const & position)
{
  if (!m_infoGetter || !m_listener)
    return;

  // Distance threshold: skip if barely moved since last check.
  if (m_currentCountry != storage::kInvalidCountryId)
  {
    if (m_lastCheckedPos.SquaredLength(position) < kDistPrecalculated)
      return;
  }

  if (bool expected = false; !m_queryInFlight.compare_exchange_strong(expected, true))
    return;

  m_lastCheckedPos = position;

  GetPlatform().RunTask(Platform::Thread::Background, [this, position]() { RunQuery(position); });
}

void LocationCountryTracker::RunQuery(m2::PointD const & position)
{
  if (!m_infoGetter)
  {
    m_queryInFlight.store(false);
    return;
  }
  auto const newCountryId = m_infoGetter->GetRegionCountryId(position);
  bool const changed = (newCountryId != m_currentCountry);

  if (changed)
  {
    m_currentCountry = newCountryId;
    m_currentCountryRect =
        (newCountryId != storage::kInvalidCountryId) ? m_infoGetter->GetLimitRectForLeaf(newCountryId) : m2::RectD();
  }

  // Mark query as done *before* posting to GUI, so new updates can schedule a new query.
  m_queryInFlight.store(false);

  if (changed)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, newCountryId]()
    {
      if (m_listener)
        m_listener(newCountryId);
    });
  }
}
