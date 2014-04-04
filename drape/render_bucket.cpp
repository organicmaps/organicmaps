#include "render_bucket.hpp"

#include "../base/stl_add.hpp"

RenderBucket::RenderBucket(TransferPointer<VertexArrayBuffer> buffer)
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

void RenderBucket::InsertUniform(TransferPointer<UniformValue> uniform)
{

}

void RenderBucket::CollectOverlayHandles()
{

}

void RenderBucket::Render()
{

}
