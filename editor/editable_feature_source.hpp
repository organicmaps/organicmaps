#pragma once

#include "indexer/feature.hpp"
#include "indexer/feature_source.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/rect2d.hpp"

#include <cstdint>
#include <functional>
#include <memory>

class EditableFeatureSource final : public FeatureSource
{
public:
  explicit EditableFeatureSource(MwmSet::MwmHandle const & handle) : FeatureSource(handle) {}

  // FeatureSource overrides:
  FeatureStatus GetFeatureStatus(uint32_t index) const override;
  bool GetModifiedFeature(uint32_t index, FeatureType & feature) const override;
  void ForEachInRectAndScale(m2::RectD const & rect, int scale,
                             std::function<void(FeatureID const &)> const & fn) const override;
  void ForEachInRectAndScale(m2::RectD const & rect, int scale,
                             std::function<void(FeatureType &)> const & fn) const override;
};

class EditableFeatureSourceFactory : public FeatureSourceFactory
{
public:
  static EditableFeatureSourceFactory const & Get()
  {
    static EditableFeatureSourceFactory const factory;
    return factory;
  }

  // FeatureSourceFactory overrides:
  std::unique_ptr<FeatureSource> operator()(MwmSet::MwmHandle const & handle) const override
  {
    return std::make_unique<EditableFeatureSource>(handle);
  }

protected:
  EditableFeatureSourceFactory() = default;
};
