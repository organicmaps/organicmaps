#pragma once

#include "drape/pointers.hpp"

class ScreenBase;

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
  TransferPointer<VertexArrayBuffer> MoveBuffer();

  size_t GetOverlayHandlesCount() const;
  TransferPointer<OverlayHandle> PopOverlayHandle();
  RefPointer<OverlayHandle> GetOverlayHandle(size_t index);
  void AddOverlayHandle(TransferPointer<OverlayHandle> handle);

  void Update(ScreenBase const & modelView);
  void CollectOverlayHandles(RefPointer<OverlayTree> tree);
  void Render(ScreenBase const & screen);

  /// Only for testing! Don't use this function in production code!
  template <typename ToDo>
  void ForEachOverlay(ToDo const & todo)
  {
    for (MasterPointer<OverlayHandle> & h : m_overlay)
      todo(h.GetRaw());
  }

private:
  vector<MasterPointer<OverlayHandle> > m_overlay;
  MasterPointer<VertexArrayBuffer> m_buffer;
};

} // namespace dp
