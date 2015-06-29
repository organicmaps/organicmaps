#include "drape_frontend/text_handle.hpp"

namespace df
{

void TextHandle::GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator,
                                      ScreenBase const & screen, bool isVisible) const
{
  ASSERT(IsValid(), ());
  UNUSED_VALUE(screen);

  TOffsetNode const & node = GetOffsetNode(gpu::TextDynamicVertex::GetDynamicStreamID());
  ASSERT(node.first.GetElementSize() == sizeof(gpu::TextDynamicVertex), ());
  ASSERT(node.second.m_count == m_normals.size(), ());

  uint32_t byteCount = m_normals.size() * sizeof(gpu::TextDynamicVertex);
  void * buffer = mutator->AllocateMutationBuffer(byteCount);
  if (isVisible)
    memcpy(buffer, m_normals.data(), byteCount);
  else
    memset(buffer, 0, byteCount);

  dp::MutateNode mutateNode;
  mutateNode.m_region = node.second;
  mutateNode.m_data = make_ref(buffer);
  mutator->AddMutation(node.first, mutateNode);
}

bool TextHandle::IndexesRequired() const
{
  return false;
}

} // namespace df
