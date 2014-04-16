#include "render_bucket.hpp"

#include "overlay_handle.hpp"
#include "vertex_array_buffer.hpp"
#include "overlay_tree.hpp"

#include "../base/stl_add.hpp"
#include "../std/bind.hpp"

RenderBucket::RenderBucket(TransferPointer<VertexArrayBuffer> buffer)
  : m_buffer(buffer)
{
}

RenderBucket::~RenderBucket()
{
  m_buffer.Destroy();
  (void)GetRangeDeletor(m_overlay, MasterPointerDeleter())();
}

RefPointer<VertexArrayBuffer> RenderBucket::GetBuffer()
{
  return m_buffer.GetRefPointer();
}

void RenderBucket::AddOverlayHandle(TransferPointer<OverlayHandle> handle)
{
  m_overlay.push_back(MasterPointer<OverlayHandle>(handle));
}

void RenderBucket::CollectOverlayHandles(RefPointer<OverlayTree> tree)
{
  for_each(m_overlay.begin(), m_overlay.end(), bind(&OverlayTree::Add, tree.GetRaw(),
                                                    bind(&MasterPointer<OverlayHandle>::GetRefPointer, _1)));
}

namespace
{

void AccumulateIndexes(MasterPointer<OverlayHandle> handle, RefPointer<IndexBufferMutator> mutator)
{
  if (handle->IsVisible())
    handle->GetElementIndexes(mutator);
}

} // namespace

void RenderBucket::Render()
{
  if (!m_overlay.empty())
  {
    // in simple case when overlay is symbol each element will be contains 6 indexes
    IndexBufferMutator mutator(6 * m_overlay.size());
    for_each(m_overlay.begin(), m_overlay.end(), bind(&AccumulateIndexes, _1,
                                                      MakeStackRefPointer(&mutator)));
    mutator.Submit(m_buffer.GetRefPointer());
  }
  m_buffer->Render();
}
