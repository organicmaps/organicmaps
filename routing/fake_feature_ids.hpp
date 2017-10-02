#pragma once

#include <cstdint>
#include <limits>

namespace routing
{
struct FakeFeatureIds
{
  static uint32_t constexpr kIndexGraphStarterId = std::numeric_limits<uint32_t>::max();
  static uint32_t constexpr kTransitGraphId = std::numeric_limits<uint32_t>::max() - 1;
};

static_assert(FakeFeatureIds::kIndexGraphStarterId != FakeFeatureIds::kTransitGraphId,
              "Fake feature ids should differ.");
}  // namespace routing
