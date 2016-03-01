#include "search/v2/mwm_context.hpp"


namespace search
{
namespace v2
{

void CoverRect(m2::RectD const & rect, int scale, covering::IntervalsT & result)
{
  covering::CoveringGetter covering(rect, covering::ViewportWithLowLevels);
  auto const & intervals = covering.Get(scale);
  result.insert(result.end(), intervals.begin(), intervals.end());
}

MwmContext::MwmContext(MwmSet::MwmHandle handle)
  : m_handle(move(handle))
  , m_value(*m_handle.GetValue<MwmValue>())
  , m_vector(m_value.m_cont, m_value.GetHeader(), m_value.m_table)
  , m_index(m_value.m_cont.GetReader(INDEX_FILE_TAG), m_value.m_factory)
{
}

void MwmContext::GetFeature(uint32_t index, FeatureType & ft) const
{
  m_vector.GetByIndex(index, ft);
  ft.SetID(FeatureID(GetId(), index));
}

}  // namespace v2
}  // namespace search
