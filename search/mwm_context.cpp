#include "search/mwm_context.hpp"

#include "indexer/cell_id.hpp"
#include "indexer/fake_feature_ids.hpp"
#include "indexer/feature_source.hpp"

namespace search
{
void CoverRect(m2::RectD const & rect, int scale, covering::Intervals & result)
{
  covering::CoveringGetter covering(rect, covering::ViewportWithLowLevels);
  auto const & intervals = covering.Get<RectId::DEPTH_LEVELS>(scale);
  result.insert(result.end(), intervals.begin(), intervals.end());
}

MwmContext::MwmContext(MwmSet::MwmHandle handle)
  : m_handle(move(handle))
  , m_value(*m_handle.GetValue<MwmValue>())
  , m_vector(m_value.m_cont, m_value.GetHeader(), m_value.m_table.get())
  , m_index(m_value.m_cont.GetReader(INDEX_FILE_TAG), m_value.m_factory)
  , m_centers(m_value)
{
}

std::unique_ptr<FeatureType> MwmContext::GetFeature(uint32_t index) const
{
  std::unique_ptr<FeatureType> ft;
  switch (GetEditedStatus(index))
  {
  case FeatureStatus::Deleted:
  case FeatureStatus::Obsolete:
    return ft;
  case FeatureStatus::Modified:
  case FeatureStatus::Created:
    ft = osm::Editor::Instance().GetEditedFeature(FeatureID(GetId(), index));
    CHECK(ft, ());
    return ft;
  case FeatureStatus::Untouched:
    ft = m_vector.GetByIndex(index);
    ft->SetID(FeatureID(GetId(), index));
    return ft;
  }
  UNREACHABLE();
}
}  // namespace search
