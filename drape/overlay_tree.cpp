#include "drape/overlay_tree.hpp"

#include "std/bind.hpp"

namespace dp
{

void OverlayTree::StartOverlayPlacing(ScreenBase const & screen, bool canOverlap)
{
  m_traits.m_modelView = screen;
  m_canOverlap = canOverlap;
  ASSERT(IsEmpty(), ());
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

  Clear();
}

} // namespace dp
