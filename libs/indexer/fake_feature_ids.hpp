#pragma once

#include <cstdint>
#include <limits>

// Before creating new fake feature interval note that routing has
// it's own fake features in routing/fake_feature_ids.hpp

namespace feature
{
struct FakeFeatureIds
{
  static bool IsEditorCreatedFeature(uint32_t id) { return id >= kEditorCreatedFeaturesStart; }

  static uint32_t constexpr k20BitsOffset = 0xfffff;
  static uint32_t constexpr kEditorCreatedFeaturesStart = std::numeric_limits<uint32_t>::max() - k20BitsOffset;
};
}  // namespace feature
