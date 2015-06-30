#include "drape_frontend/text_handle.hpp"

namespace df
{

TextHandle::TextHandle(FeatureID const & id, dp::Anchor anchor, double priority)
  : OverlayHandle(id, anchor, priority)
  , m_forceUpdateNormals(false)
  , m_isLastVisible(false)
{}

TextHandle::TextHandle(FeatureID const & id, dp::Anchor anchor, double priority,
           gpu::TTextDynamicVertexBuffer && normals)
  : OverlayHandle(id, anchor, priority)
  , m_normals(move(normals))
  , m_forceUpdateNormals(false)
  , m_isLastVisible(false)
{}

void TextHandle::GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator,
                                      ScreenBase const & screen) const
{
  ASSERT(IsValid(), ());
  UNUSED_VALUE(screen);

  bool const isVisible = IsVisible();
  if (!m_forceUpdateNormals && m_isLastVisible == isVisible)
    return;

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

  m_isLastVisible = isVisible;
}

bool TextHandle::IndexesRequired() const
{
  return false;
}

void TextHandle::SetForceUpdateNormals(bool forceUpdate) const
{
  m_forceUpdateNormals = forceUpdate;
}

} // namespace df
