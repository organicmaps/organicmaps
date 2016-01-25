#include "search/v2/mwm_context.hpp"

#include "indexer/index.hpp"

namespace search
{
namespace v2
{
MwmContext::MwmContext(MwmSet::MwmHandle handle)
  : m_handle(move(handle))
  , m_value(*m_handle.GetValue<MwmValue>())
  , m_id(m_handle.GetId())
  , m_vector(m_value.m_cont, m_value.GetHeader(), m_value.m_table)
  , m_index(m_value.m_cont.GetReader(INDEX_FILE_TAG), m_value.m_factory)
{
}

string const & MwmContext::GetMwmName() const
{
  return m_id.GetInfo()->GetCountryName();
}

}  // namespace v2
}  // namespace search
