#include "search/v2/mwm_context.hpp"


namespace search
{
namespace v2
{
MwmContext::MwmContext(MwmSet::MwmHandle handle)
  : m_handle(move(handle))
  , m_value(*m_handle.GetValue<MwmValue>())
  , m_vector(m_value.m_cont, m_value.GetHeader(), m_value.m_table)
  , m_index(m_value.m_cont.GetReader(INDEX_FILE_TAG), m_value.m_factory)
{
}

shared_ptr<MwmInfo> const & MwmContext::GetInfo() const { return GetId().GetInfo(); }
}  // namespace v2
}  // namespace search
