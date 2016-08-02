#include "map/displacement_mode_manager.hpp"

DisplacementModeManager::DisplacementModeManager(TCallback && callback)
  : m_callback(move(callback)), m_mask(0)
{
}

void DisplacementModeManager::Set(Slot slot, bool show)
{
  lock_guard<mutex> lock(m_mu);

  uint32_t const bit = static_cast<uint32_t>(1) << slot;
  uint32_t const mask = m_mask;
  if (show)
    m_mask = mask | bit;
  else
    m_mask = mask & ~bit;

  if (mask == m_mask)
    return;

  if (m_mask != 0)
    m_callback(true /* show */);
  else
    m_callback(false /* show */);
}
