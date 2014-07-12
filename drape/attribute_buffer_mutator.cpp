#include "attribute_buffer_mutator.hpp"

void AttributeBufferMutator::AddMutation(BindingInfo const & info, MutateNode const & node)
{
  m_data[info].push_back(node);
}
