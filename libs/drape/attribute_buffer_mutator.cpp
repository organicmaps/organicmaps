#include "drape/attribute_buffer_mutator.hpp"

namespace dp
{
AttributeBufferMutator::~AttributeBufferMutator()
{
  SharedBufferManager & mng = SharedBufferManager::instance();
  for (size_t i = 0; i < m_array.size(); ++i)
  {
    TBufferNode & node = m_array[i];
    mng.freeSharedBuffer(node.second, node.first);
  }
}

void AttributeBufferMutator::AddMutation(BindingInfo const & info, MutateNode const & node)
{
  m_data[info].push_back(node);
}

void * AttributeBufferMutator::AllocateMutationBuffer(size_t byteCount)
{
  m_array.push_back(make_pair(SharedBufferManager::instance().reserveSharedBuffer(byteCount), byteCount));
  return &((*m_array.back().first)[0]);
}
}  // namespace dp
