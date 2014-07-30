#include "attribute_buffer_mutator.hpp"

namespace dp
{

void AttributeBufferMutator::AddMutation(BindingInfo const & info, MutateNode const & node)
{
  m_data[info].push_back(node);
}

} // namespace dp
