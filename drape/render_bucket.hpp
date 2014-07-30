#pragma once

#include "pointers.hpp"


namespace dp
{

class OverlayHandle;
class OverlayTree;
class VertexArrayBuffer;

class RenderBucket
{
public:
  RenderBucket(TransferPointer<VertexArrayBuffer> buffer);
  ~RenderBucket();

  RefPointer<VertexArrayBuffer> GetBuffer();

  void AddOverlayHandle(TransferPointer<OverlayHandle> handle);

  void CollectOverlayHandles(RefPointer<OverlayTree> tree);
  void Render();

private:
  vector<MasterPointer<OverlayHandle> > m_overlay;
  MasterPointer<VertexArrayBuffer> m_buffer;
};

} // namespace dp
