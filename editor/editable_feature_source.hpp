#pragma once

#include "indexer/feature_source.hpp"

namespace datasource
{
class EditableFeatureSource : public FeatureSource
{
public:
  EditableFeatureSource(MwmSet::MwmHandle const & handle) : FeatureSource(handle) {}

  FeatureStatus GetFeatureStatus(uint32_t index) const override;

  bool GetModifiedFeature(uint32_t index, FeatureType & feature) const override;

  void ForEachInRectAndScale(m2::RectD const & rect, int scale,
                             std::function<void(FeatureID const &)> const & fn) override;
  void ForEachInRectAndScale(m2::RectD const & rect, int scale,
                             std::function<void(FeatureType &)> const & fn) override;

};  // class EditableFeatureSource
}  // namespace datasource
