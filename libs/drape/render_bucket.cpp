#include "drape/render_bucket.hpp"

#include "drape/attribute_buffer_mutator.hpp"
#include "drape/debug_renderer.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/overlay_tree.hpp"
#include "drape/vertex_array_buffer.hpp"

#include <optional>

namespace dp
{
RenderBucket::RenderBucket(drape_ptr<VertexArrayBuffer> && buffer) : m_buffer(std::move(buffer)) {}

RenderBucket::~RenderBucket() {}

ref_ptr<VertexArrayBuffer> RenderBucket::GetBuffer()
{
  return make_ref(m_buffer);
}

drape_ptr<VertexArrayBuffer> && RenderBucket::MoveBuffer()
{
  return std::move(m_buffer);
}

size_t RenderBucket::GetOverlayHandlesCount() const
{
  return m_overlay.size();
}

#ifdef DEBUG
void RenderBucket::CheckOverlayCounters() const
{
  // Check counters correctness.
  // Caller should fill attributes first, and call AddOverlayHandle then.

  size_t indexedOverlayCount = 0, dynamicOverlayCount = 0;
  for (auto const & handle : m_overlay)
  {
    if (handle->IndexesRequired())
      ++indexedOverlayCount;
    if (handle->HasDynamicAttributes())
      ++dynamicOverlayCount;
  }

  ASSERT_EQUAL(m_indexedOverlayCount, indexedOverlayCount, (m_overlay.size()));
  ASSERT_EQUAL(m_dynamicOverlayCount, dynamicOverlayCount, (m_overlay.size()));
}
#endif

drape_ptr<OverlayHandle> RenderBucket::PopOverlayHandle()
{
  ASSERT(!m_overlay.empty(), ());
  if (m_overlay.back()->IndexesRequired())
  {
    ASSERT_GREATER(m_indexedOverlayCount, 0, (m_overlay.size()));
    --m_indexedOverlayCount;
  }
  if (m_overlay.back()->HasDynamicAttributes())
  {
    ASSERT_GREATER(m_dynamicOverlayCount, 0, (m_overlay.size()));
    --m_dynamicOverlayCount;
  }
  auto h = std::move(m_overlay.back());
  m_overlay.pop_back();
  return h;
}

ref_ptr<OverlayHandle> RenderBucket::GetOverlayHandle(size_t index)
{
  return make_ref(m_overlay[index]);
}

void RenderBucket::AddOverlayHandle(drape_ptr<OverlayHandle> && handle)
{
  if (handle->IndexesRequired())
    ++m_indexedOverlayCount;
  if (handle->HasDynamicAttributes())
    ++m_dynamicOverlayCount;
  m_overlay.push_back(std::move(handle));
}

void RenderBucket::BeforeUpdate()
{
  for (auto & overlayHandle : m_overlay)
    overlayHandle->BeforeUpdate();
}

void RenderBucket::Update(ScreenBase const & modelView)
{
  BeforeUpdate();
  for (auto & overlayHandle : m_overlay)
    if (overlayHandle->IsVisible())
      overlayHandle->Update(modelView);
}

void RenderBucket::CollectOverlayHandles(ref_ptr<OverlayTree> tree)
{
  BeforeUpdate();
  for (auto const & overlayHandle : m_overlay)
    tree->Add(make_ref(overlayHandle));
}

bool RenderBucket::HasOverlayHandles() const
{
  return !m_overlay.empty();
}

bool RenderBucket::RemoveOverlayHandles(ref_ptr<OverlayTree> tree)
{
  for (auto const & overlayHandle : m_overlay)
    if (tree->Remove(make_ref(overlayHandle)))
      return true;
  return false;
}

void RenderBucket::SetOverlayVisibility(bool isVisible)
{
  for (auto const & overlayHandle : m_overlay)
    overlayHandle->SetIsVisible(isVisible);
}

void RenderBucket::Render(ref_ptr<GraphicsContext> context, bool drawAsLine)
{
  ASSERT(m_buffer != nullptr, ());
#ifdef DEBUG
  CheckOverlayCounters();
#endif

  if (!m_overlay.empty() && (m_indexedOverlayCount != 0 || m_dynamicOverlayCount != 0))
  {
    // in simple case when overlay is symbol each element will be contains 6 indexes
    AttributeBufferMutator attributeMutator;
    std::optional<IndexBufferMutator> indexMutator;
    ref_ptr<IndexBufferMutator> rfpIndex;
    if (m_indexedOverlayCount != 0)
    {
      indexMutator.emplace(static_cast<uint32_t>(6 * m_indexedOverlayCount));
      rfpIndex = make_ref(&*indexMutator);
    }
    ref_ptr<AttributeBufferMutator> rfpAttrib = make_ref(&attributeMutator);

    bool hasIndexMutation = false;
    for (drape_ptr<OverlayHandle> const & handle : m_overlay)
    {
      if (m_indexedOverlayCount != 0 && handle->IndexesRequired())
      {
        if (handle->IsVisible())
          handle->GetElementIndexes(rfpIndex);
        hasIndexMutation = true;
      }

      if (m_dynamicOverlayCount != 0 && handle->HasDynamicAttributes())
        handle->GetAttributeMutation(rfpAttrib);
    }

    bool const hasAttributeMutation = !attributeMutator.IsEmpty();
    if (hasIndexMutation || hasAttributeMutation)
      m_buffer->ApplyMutation(context, hasIndexMutation ? rfpIndex : nullptr,
                              hasAttributeMutation ? rfpAttrib : nullptr);
  }
  m_buffer->Render(context, drawAsLine);
}

void RenderBucket::SetFeatureMinZoom(int minZoom)
{
  if (minZoom < m_featuresMinZoom)
    m_featuresMinZoom = minZoom;
}

void RenderBucket::RenderDebug(ref_ptr<GraphicsContext> context, ScreenBase const & screen,
                               ref_ptr<DebugRenderer> debugRectRenderer) const
{
  if (!debugRectRenderer || !debugRectRenderer->IsEnabled() || m_overlay.empty())
    return;

  for (auto const & handle : m_overlay)
  {
    if (!screen.PixelRect().IsIntersect(handle->GetPixelRect(screen, false)))
      continue;

    auto const & rects = handle->GetExtendedPixelShape(screen);
    for (auto const & rect : rects)
    {
      if (screen.isPerspective() && !screen.PixelRectIn3d().IsIntersect(m2::RectD(rect)))
        continue;

      auto color = dp::Color::Green();
      if (!handle->IsVisible())
        color = handle->IsReady() ? dp::Color::Red() : dp::Color::Yellow();

      debugRectRenderer->DrawRect(context, screen, rect, color);
    }
  }
}
}  // namespace dp
