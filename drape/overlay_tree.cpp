#include "overlay_tree.hpp"

#include "../std/bind.hpp"

namespace dp
{

void OverlayTree::StartOverlayPlacing(ScreenBase const & screen, bool canOverlap)
{
  m_traits.m_modelView = screen;
  m_canOverlap = canOverlap;
  ASSERT(IsEmpty(), ());
}

void OverlayTree::Add(RefPointer<OverlayHandle> handle)
{
  ScreenBase const & modelView = GetModelView();

  handle->SetIsVisible(m_canOverlap);
  handle->Update(modelView);

  if (!handle->IsValid())
    return;
  
  m2::RectD const pixelRect = handle->GetPixelRect(modelView);

  typedef buffer_vector<RefPointer<OverlayHandle>, 8> OverlayContainerT;
  OverlayContainerT elements;
  /*
   * Find elements that already on OverlayTree and it's pixel rect
   * intersect with handle pixel rect ("Intersected elements")
   */
  ForEachInRect(pixelRect, [&] (RefPointer<OverlayHandle> r)
  {
    if (handle->IsIntersect(modelView, *r.GetRaw()))
      elements.push_back(r);
  });

  double const inputPriority = handle->GetPriority();
  /*
   * In this loop we decide which element must be visible
   * If input element "handle" more priority than all "Intersected elements"
   * than we remove all "Intersected elements" and insert input element "handle"
   * But if some of already inserted elements more priority than we don't insert "handle"
   */
  for (OverlayContainerT::const_iterator it = elements.begin(); it != elements.end(); ++it)
    if (inputPriority < (*it)->GetPriority())
      return;

  for (OverlayContainerT::const_iterator it = elements.begin(); it != elements.end(); ++it)
    Erase(*it);

  BaseT::Add(handle, pixelRect);
}

void OverlayTree::EndOverlayPlacing()
{
  ForEach([] (RefPointer<OverlayHandle> handle)
  {
    handle->SetIsVisible(true);
  });

  Clear();
}

} // namespace dp
