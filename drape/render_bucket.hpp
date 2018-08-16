#pragma once

#include "drape/pointers.hpp"

#include "std/function.hpp"
#include "std/limits.hpp"
#include "std/vector.hpp"

class ScreenBase;

namespace df
{
class BatchMergeHelper;
}

namespace dp
{

class GraphicsContext;
class DebugRenderer;
class OverlayHandle;
class OverlayTree;
class VertexArrayBuffer;

class RenderBucket
{
  friend class df::BatchMergeHelper;
public:
  RenderBucket(drape_ptr<VertexArrayBuffer> && buffer);
  ~RenderBucket();

  ref_ptr<VertexArrayBuffer> GetBuffer();
  drape_ptr<VertexArrayBuffer> && MoveBuffer();

  size_t GetOverlayHandlesCount() const;
  drape_ptr<OverlayHandle> PopOverlayHandle();
  ref_ptr<OverlayHandle> GetOverlayHandle(size_t index);
  void AddOverlayHandle(drape_ptr<OverlayHandle> && handle);

  void Update(ScreenBase const & modelView);
  void CollectOverlayHandles(ref_ptr<OverlayTree> tree);
  bool HasOverlayHandles() const;
  void RemoveOverlayHandles(ref_ptr<OverlayTree> tree);
  void SetOverlayVisibility(bool isVisible);
  void Render(bool drawAsLine);

  // Only for testing! Don't use this function in production code!
  void RenderDebug(ref_ptr<GraphicsContext> context, ScreenBase const & screen,
                   ref_ptr<DebugRenderer> debugRectRenderer) const;

  // Only for testing! Don't use this function in production code!
  template <typename ToDo>
  void ForEachOverlay(ToDo const & todo)
  {
    for (drape_ptr<OverlayHandle> const & h : m_overlay)
      todo(make_ref(h));
  }

  void SetFeatureMinZoom(int minZoom);
  int GetMinZoom() const { return m_featuresMinZoom; }

private:
  void BeforeUpdate();

private:
  int m_featuresMinZoom = numeric_limits<int>::max();

  vector<drape_ptr<OverlayHandle> > m_overlay;
  drape_ptr<VertexArrayBuffer> m_buffer;
};

} // namespace dp
