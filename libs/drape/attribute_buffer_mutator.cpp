#include "drape/attribute_buffer_mutator.hpp"

namespace dp
{
AttributeBufferMutator::~AttributeBufferMutator()
{
  SharedBufferManager & mng = SharedBufferManager::Instance();
  for (size_t i = 0; i < m_array.size(); ++i)
  {
    TBufferNode & node = m_array[i];
    mng.FreeSharedBuffer(node.second, std::move(node.first));
  }
}

void AttributeBufferMutator::AddMutation(BindingInfo const & info, MutateRegion region, ref_ptr<void> data)
{
  m_data[info].emplace_back(region, data);
}

void * AttributeBufferMutator::AllocateMutationBuffer(size_t byteCount)
{
  m_array.push_back(make_pair(SharedBufferManager::Instance().ReserveSharedBuffer(byteCount), byteCount));
  return m_array.back().first->data();
}
}  // namespace dp
