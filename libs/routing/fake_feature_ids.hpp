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

  static bool IsGuidesFeature(uint32_t id)
  {
    return id >= kGuidesGraphFeaturesStart && id < kTransitGraphFeaturesStart;
  }

  static uint32_t constexpr kIndexGraphStarterId = std::numeric_limits<uint32_t>::max();
  // It's important that |kGuidesGraphFeaturesStart| is greater than maximum number of real road
  // feature id. Also transit and guides fake feature id should not overlap with real feature id
  // and editor feature ids.
  static uint32_t constexpr k28BitsOffset = 0xfffffff;
  static uint32_t constexpr k24BitsOffset = 0xffffff;
  static uint32_t constexpr kTransitGraphFeaturesStart = std::numeric_limits<uint32_t>::max() - k28BitsOffset;
  static uint32_t constexpr kGuidesGraphFeaturesStart = kTransitGraphFeaturesStart - k24BitsOffset;
};

static_assert(feature::FakeFeatureIds::kEditorCreatedFeaturesStart > FakeFeatureIds::kTransitGraphFeaturesStart,
              "routing::FakeFeatureIds::kTransitGraphFeaturesStart or "
              "feature::FakeFeatureIds::kEditorCreatedFeaturesStart was changed. "
              "Interval for transit fake features may be too small.");

static_assert(FakeFeatureIds::kTransitGraphFeaturesStart > FakeFeatureIds::kGuidesGraphFeaturesStart,
              "routing::FakeFeatureIds::kTransitGraphFeaturesStart or "
              "feature::FakeFeatureIds::kGuidesGraphFeaturesStart was changed. "
              "Interval for guides fake features may be too small.");

static_assert(feature::FakeFeatureIds::kEditorCreatedFeaturesStart - FakeFeatureIds::kTransitGraphFeaturesStart >=
                  FakeFeatureIds::k24BitsOffset - feature::FakeFeatureIds::k20BitsOffset,
              "routing::FakeFeatureIds::kTransitGraphFeaturesStart or "
              "feature::FakeFeatureIds::kEditorCreatedFeaturesStart was changed. "
              "Interval for transit fake features may be too small.");
}  // namespace routing
