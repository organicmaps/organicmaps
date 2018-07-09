#pragma once

#include "indexer/fake_feature_ids.hpp"

#include <cstdint>
#include <limits>

namespace routing
{
struct FakeFeatureIds
{
  static bool IsTransitFeature(uint32_t id)
  {
    return id >= kTransitGraphFeaturesStart && id != kIndexGraphStarterId &&
           !feature::FakeFeatureIds::IsEditorCreatedFeature(id);
  }

  static uint32_t constexpr kIndexGraphStarterId = std::numeric_limits<uint32_t>::max();
  // It's important that |kTransitGraphFeaturesStart| is greater than maximum number of real road
  // feature id. Also transit fake feature id should not overlap with real feature id and editor
  // feature ids.
  static uint32_t constexpr k24BitsOffset = 0xffffff;
  static uint32_t constexpr kTransitGraphFeaturesStart =
      std::numeric_limits<uint32_t>::max() - k24BitsOffset;
};

static_assert(feature::FakeFeatureIds::kEditorCreatedFeaturesStart >
                      FakeFeatureIds::kTransitGraphFeaturesStart &&
                  feature::FakeFeatureIds::kEditorCreatedFeaturesStart -
                          FakeFeatureIds::kTransitGraphFeaturesStart >=
                      FakeFeatureIds::k24BitsOffset - feature::FakeFeatureIds::k20BitsOffset,
              "routing::FakeFeatureIds::kTransitGraphFeaturesStart or "
              "feature::FakeFeatureIds::kEditorCreatedFeaturesStart was changed. "
              "Interval for transit fake features may be too small.");
}  // namespace routing
