#include "editor/editable_feature_source.hpp"

#include "editor/osm_editor.hpp"

FeatureStatus EditableFeatureSource::GetFeatureStatus(uint32_t index) const
{
  osm::Editor & editor = osm::Editor::Instance();
  return editor.GetFeatureStatus(m_handle.GetId(), index);
}

bool EditableFeatureSource::GetModifiedFeature(uint32_t index, FeatureType & feature) const
{
  osm::Editor & editor = osm::Editor::Instance();
  return editor.GetEditedFeature(m_handle.GetId(), index, feature);
}

void EditableFeatureSource::ForEachAdditionalFeature(m2::RectD const & rect, int scale,
                                                     std::function<void(uint32_t)> const & fn) const
{
  osm::Editor & editor = osm::Editor::Instance();
  editor.ForEachCreatedFeature(m_handle.GetId(), fn, rect, scale);
}
