#include "editor/editable_feature_source.hpp"

#include "editor/osm_editor.hpp"

EditableFeatureSource::EditableFeatureSource(MwmSet::MwmHandle const & handle)
  : FeatureSource(handle)
  , m_editor(osm::Editor::Instance())
{}

FeatureStatus EditableFeatureSource::GetFeatureStatus(uint32_t index) const
{
  return m_editor.GetFeatureStatus(m_handle.GetId(), index);
}

std::unique_ptr<FeatureType> EditableFeatureSource::GetModifiedFeature(uint32_t index) const
{
  auto const emo = m_editor.GetEditedFeature(FeatureID(m_handle.GetId(), index));
  if (emo)
    return FeatureType::CreateFromMapObject(*emo);
  return {};
}

void EditableFeatureSource::ForEachAdditionalFeature(m2::RectD const & rect, int scale,
                                                     std::function<void(uint32_t)> const & fn) const
{
  m_editor.ForEachCreatedFeature(m_handle.GetId(), fn, rect, scale);
}
