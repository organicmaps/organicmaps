#include "search/v2/mwm_context.hpp"

#include "indexer/index.hpp"

namespace search
{
namespace v2
{
MwmContext::MwmContext(MwmValue & value, MwmSet::MwmId const & id)
  : m_value(value), m_id(id), m_vector(m_value.m_cont, m_value.GetHeader(), m_value.m_table)
{
}
}  // namespace v2
}  // namespace search
