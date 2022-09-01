#include "editor/editable_feature_source.hpp"

#include "editor/osm_editor.hpp"

FeatureStatus EditableFeatureSource::GetFeatureStatus(uint32_t index) const
{
  osm::Editor & editor = osm::Editor::Instance();
  return editor.GetFeatureStatus(m_handle.GetId(), index);
}

std::unique_ptr<FeatureType> EditableFeatureSource::GetModifiedFeature(uint32_t index) const
{
  osm::Editor & editor = osm::Editor::Instance();
  auto const emo = editor.GetEditedFeature(FeatureID(m_handle.GetId(), index));
  if (emo)
    return FeatureType::CreateFromMapObject(*emo);
  return {};
}

void EditableFeatureSource::ForEachAdditionalFeature(m2::RectD const & rect, int scale,
                                                     std::function<void(uint32_t)> const & fn) const
{
  osm::Editor & editor = osm::Editor::Instance();
  editor.ForEachCreatedFeature(m_handle.GetId(), fn, rect, scale);
}
