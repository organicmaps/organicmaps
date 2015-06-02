#include "drape/overlay_tree.hpp"

#include "std/bind.hpp"

namespace dp
{

int const FRAME_UPDATE_PERIOD = 10;

void OverlayTree::Frame()
{
  m_frameCounter++;
  if (m_frameCounter >= FRAME_UPDATE_PERIOD)
    m_frameCounter = -1;
}

bool OverlayTree::IsNeedUpdate() const
{
  return m_frameCounter == -1;
}

void OverlayTree::ForceUpdate()
{
  Clear();
  m_frameCounter = -1;
}

void OverlayTree::StartOverlayPlacing(ScreenBase const & screen, bool canOverlap)
{
  ASSERT(IsNeedUpdate(), ());
  Clear();
  m_traits.m_modelView = screen;
  m_canOverlap = canOverlap;
}

void OverlayTree::Add(ref_ptr<OverlayHandle> handle, bool isTransparent)
{
  ScreenBase const & modelView = GetModelView();

  handle->SetIsVisible(m_canOverlap);
  handle->Update(modelView);

  if (!handle->IsValid())
    return;

  m2::RectD const pixelRect = handle->GetPixelRect(modelView);
  if (!m_traits.m_modelView.PixelRect().IsIntersect(pixelRect))
  {
    handle->SetIsVisible(false);
    return;
  }

  typedef buffer_vector<detail::OverlayInfo, 8> OverlayContainerT;
  OverlayContainerT elements;
  /*
   * Find elements that already on OverlayTree and it's pixel rect
   * intersect with handle pixel rect ("Intersected elements")
   */
  ForEachInRect(pixelRect, [&] (detail::OverlayInfo const & info)
  {
    if (isTransparent == info.m_isTransparent && handle->IsIntersect(modelView, info.m_handle))
      elements.push_back(info);
  });

  double const inputPriority = handle->GetPriority();
  /*
   * In this loop we decide which element must be visible
   * If input element "handle" more priority than all "Intersected elements"
   * than we remove all "Intersected elements" and insert input element "handle"
   * But if some of already inserted elements more priority than we don't insert "handle"
   */
  for (OverlayContainerT::const_iterator it = elements.begin(); it != elements.end(); ++it)
    if (inputPriority < it->m_handle->GetPriority())
      return;

  for (OverlayContainerT::const_iterator it = elements.begin(); it != elements.end(); ++it)
    Erase(*it);

  BaseT::Add(detail::OverlayInfo(handle, isTransparent), pixelRect);
}

void OverlayTree::EndOverlayPlacing()
{
  ForEach([] (detail::OverlayInfo const & info)
  {
    info.m_handle->SetIsVisible(true);
  });
}

void OverlayTree::Select(m2::RectD const & rect, TSelectResult & result) const
{
  ScreenBase screen = m_traits.m_modelView;
  ForEachInRect(rect, [&](detail::OverlayInfo const & info)
  {
    if (info.m_handle->IsValid() &&
        info.m_handle->IsVisible() &&
        info.m_handle->GetFeatureID().IsValid())
    {
      OverlayHandle::Rects shape;
      info.m_handle->GetPixelShape(screen, shape);
      for (m2::RectF const & rShape : shape)
      {
        if (rShape.IsIntersect(m2::RectF(rect)))
        {
          result.push_back(info.m_handle);
          break;
        }
      }
    }
  });
}

} // namespace dp
