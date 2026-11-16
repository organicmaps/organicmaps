#pragma once

#include "indexer/mwm_set.hpp"

#include <cstdint>
#include <limits>
#include <tuple>

namespace dp
{
// Identifies a subway/transit interchange (a transfer) within an MWM. Kept outside OverlayID because
// OverlayID drives overlay de-duplication and rival rejection; it mirrors routing::transit::TransferId.
struct TransitTransferId
{
  MwmSet::MwmId m_mwmId;
  uint64_t m_transferId = std::numeric_limits<uint64_t>::max();

  bool IsValid() const { return m_mwmId.IsAlive() && m_transferId != std::numeric_limits<uint64_t>::max(); }

  bool operator==(TransitTransferId const & rhs) const
  {
    return m_mwmId == rhs.m_mwmId && m_transferId == rhs.m_transferId;
  }
  bool operator!=(TransitTransferId const & rhs) const { return !operator==(rhs); }
  bool operator<(TransitTransferId const & rhs) const
  {
    return std::tie(m_mwmId, m_transferId) < std::tie(rhs.m_mwmId, rhs.m_transferId);
  }
};
}  // namespace dp
