#include "search/mwm_context.hpp"

#include "indexer/cell_id.hpp"

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

bool MwmContext::GetFeature(uint32_t index, FeatureType & ft) const
{
  switch (GetEditedStatus(index))
  {
  case osm::Editor::FeatureStatus::Deleted:
  case osm::Editor::FeatureStatus::Obsolete:
    return false;
  case osm::Editor::FeatureStatus::Modified:
  case osm::Editor::FeatureStatus::Created:
    VERIFY(osm::Editor::Instance().GetEditedFeature(GetId(), index, ft), ());
    return true;
  case osm::Editor::FeatureStatus::Untouched:
    m_vector.GetByIndex(index, ft);
    ft.SetID(FeatureID(GetId(), index));
    return true;
  }
}

bool MwmContext::GetStreetIndex(uint32_t houseId, uint32_t & streetId)
{
  if (!m_houseToStreetTable)
  {
    m_houseToStreetTable = HouseToStreetTable::Load(m_value);
    ASSERT(m_houseToStreetTable, ());
  }
  return m_houseToStreetTable->Get(houseId, streetId);
}
}  // namespace search
