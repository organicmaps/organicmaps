#include "search/mwm_context.hpp"
#include "search/house_to_street_table.hpp"

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
  : m_handle(std::move(handle))
  , m_value(*m_handle.GetValue())
  , m_vector(m_value.m_cont, m_value.GetHeader(), m_value.m_ftTable.get(), m_value.m_relTable.get(),
             m_value.m_metaDeserializer.get())
  , m_index(m_value.m_cont.GetReader(INDEX_FILE_TAG))
  , m_centers(m_value)
  , m_editableSource(m_handle)
{}

MwmContext::MwmContext(MwmSet::MwmHandle handle, MwmType type) : MwmContext(std::move(handle))
{
  m_type = type;
}

std::unique_ptr<FeatureType> MwmContext::GetFeature(uint32_t index) const
{
  std::unique_ptr<FeatureType> ft;
  switch (GetEditedStatus(index))
  {
  case FeatureStatus::Deleted:
  case FeatureStatus::Obsolete: return ft;
  case FeatureStatus::Modified:
  case FeatureStatus::Created:
    ft = m_editableSource.GetModifiedFeature(index);
    CHECK(ft, ());
    return ft;
  case FeatureStatus::Untouched:
    auto ft = m_vector.GetByIndex(index);
    CHECK(ft, ());
    ft->SetID(FeatureID(GetId(), index));
    return ft;
  }
  UNREACHABLE();
}

std::optional<uint32_t> MwmContext::GetStreet(uint32_t index) const
{
  /// @todo Should store and fetch parent street id in Editor (now it has only name).
  if (feature::FakeFeatureIds::IsEditorCreatedFeature(index))
    return {};

  if (!m_value.m_house2street)
    m_value.m_house2street = LoadHouseToStreetTable(m_value);

  auto const res = m_value.m_house2street->Get(index);
  if (res)
  {
    ASSERT(res->m_type == HouseToStreetTable::StreetIdType::FeatureId, ());
    return res->m_streetId;
  }
  return {};
}

}  // namespace search
