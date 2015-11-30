#include "drape_frontend/text_handle.hpp"

#include "drape/texture_manager.hpp"

namespace df
{

TextHandle::TextHandle(FeatureID const & id, strings::UniString const & text,
                       dp::Anchor anchor, uint64_t priority,
                       ref_ptr<dp::TextureManager> textureManager)
  : OverlayHandle(id, anchor, priority)
  , m_forceUpdateNormals(false)
  , m_isLastVisible(false)
  , m_text(text)
  , m_textureManager(textureManager)
  , m_glyphsReady(false)
{}

TextHandle::TextHandle(FeatureID const & id, strings::UniString const & text,
                       dp::Anchor anchor, uint64_t priority,
                       ref_ptr<dp::TextureManager> textureManager,
                       gpu::TTextDynamicVertexBuffer && normals)
  : OverlayHandle(id, anchor, priority)
  , m_normals(move(normals))
  , m_forceUpdateNormals(false)
  , m_isLastVisible(false)
  , m_text(text)
  , m_textureManager(textureManager)
  , m_glyphsReady(false)
{}

void TextHandle::GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator,
                                      ScreenBase const & screen) const
{
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

bool TextHandle::Update(ScreenBase const & screen)
{
  if (!m_glyphsReady)
    m_glyphsReady = m_textureManager->AreGlyphsReady(m_text);

  return m_glyphsReady;
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
