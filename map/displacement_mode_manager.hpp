#pragma once

#include <climits>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>

// This class should be used as a single point to control whether
// hotels displacement mode should be activated or not.
//
// *NOTE* as the class is thread-safe, the callback should be
// very-very lightweight and it should be guaranteed that callback or
// its callees do not call DisplacementModeManager API. Otherwise,
// deadlock or exception may happen.
class DisplacementModeManager
{
public:
  using TCallback = std::function<void(bool show)>;

  enum Slot
  {
    SLOT_INTERACTIVE_SEARCH,
    SLOT_MAP_SELECTION,
    SLOT_DEBUG,
    SLOT_COUNT
  };

  DisplacementModeManager(TCallback && callback);

  void Set(Slot slot, bool show);

private:
  TCallback m_callback;

  std::mutex m_mu;
  uint32_t m_mask;

  static_assert(SLOT_COUNT <= sizeof(m_mask) * CHAR_BIT, "Number of slots is too large");
};
