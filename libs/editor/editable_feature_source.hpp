#pragma once

#include "indexer/feature_source.hpp"

namespace osm
{
class Editor;
}  // namespace osm

class EditableFeatureSource final : public FeatureSource
{
  osm::Editor const & m_editor;

public:
  explicit EditableFeatureSource(MwmSet::MwmHandle const & handle);

  // FeatureSource overrides:
  FeatureStatus GetFeatureStatus(uint32_t index) const override;
  std::unique_ptr<FeatureType> GetModifiedFeature(uint32_t index) const override;
  void ForEachAdditionalFeature(m2::RectD const & rect, int scale,
                                std::function<void(uint32_t)> const & fn) const override;
};

class EditableFeatureSourceFactory : public FeatureSourceFactory
{
public:
  // FeatureSourceFactory overrides:
  std::unique_ptr<FeatureSource> operator()(MwmSet::MwmHandle const & handle) const override
  {
    return std::make_unique<EditableFeatureSource>(handle);
  }
};
