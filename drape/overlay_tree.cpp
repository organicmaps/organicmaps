#include "overlay_tree.hpp"

#include "../std/bind.hpp"

namespace dp
{

void OverlayTree::StartOverlayPlacing(ScreenBase const & screen, bool canOverlap)
{
  m_modelView = screen;
  m_canOverlap = canOverlap;
  ASSERT(m_tree.empty(), ());
}

void OverlayTree::Add(RefPointer<OverlayHandle> handle)
{
  handle->SetIsVisible(m_canOverlap);
  handle->Update(m_modelView);

  if (!handle->IsValid())
    return;

  m2::RectD pixelRect = handle->GetPixelRect(m_modelView);
  
  find_result_t elements;
  /*
   * Find elements that already on OverlayTree and it's pixel rect
   * intersect with handle pixel rect ("Intersected elements")
   */
  FindIntersectedFunctor f(pixelRect, elements);
  m_tree.for_each(f);

  double inputPriority = handle->GetPriority();
  /*
   * In this loop we decide which element must be visible
   * If input element "handle" more priority than all "Intersected elements"
   * than we remove all "Intersected elements" and insert input element "handle"
   * But if some of already inserted elements more priority than we don't insert "handle"
   */
  for (find_result_t::const_iterator it = elements.begin(); it != elements.end(); ++it)
  {
    if (inputPriority < (*it)->m_nodeValue->GetPriority())
      return;
  }

  for (find_result_t::const_iterator it = elements.begin(); it != elements.end(); ++it)
    m_tree.erase(*(*it));

  m_tree.insert(Node(handle, pixelRect));
}

void OverlayTree::EndOverlayPlacing()
{
  for (tree_t::const_iterator it = m_tree.begin(); it != m_tree.end(); ++it)
  {
    RefPointer<OverlayHandle> handle = (*it).m_nodeValue;
    handle->SetIsVisible(true);
  }
  m_tree.clear();
}

////////////////////////////////////////////////
OverlayTree::Node::Node(RefPointer<OverlayHandle> handle,
                        m2::RectD const & pixelRect)
  : m_nodeValue(handle)
{
  m_pts[0] = pixelRect.minX();
  m_pts[1] = pixelRect.minY();
  m_pts[2] = pixelRect.maxX();
  m_pts[3] = pixelRect.maxY();
}

OverlayTree::BaseFindFunctor::BaseFindFunctor(m2::RectD const & r)
  : m_rect(r)
{
}

bool OverlayTree::BaseFindFunctor::ScanLeft(size_t plane, Node const & v) const
{
  switch (plane & 3)    // % 4
  {
  case 2: return m_rect.minX() < v[2];
  case 3: return m_rect.minY() < v[3];
  default: return true;
  }
}

bool OverlayTree::BaseFindFunctor::ScanRight(size_t plane, Node const & v) const
{
  switch (plane & 3)  // % 4
  {
  case 0: return m_rect.maxX() > v[0];
  case 1: return m_rect.maxY() > v[1];
  default: return true;
  }
}

OverlayTree::FindIntersectedFunctor::FindIntersectedFunctor(m2::RectD const & r,
                                                            find_result_t & intersections)
  : base_t(r)
  , m_intersections(intersections)
{
}

void OverlayTree::FindIntersectedFunctor::operator()(OverlayTree::Node const & node)
{
  m2::RectD const & r = base_t::m_rect;

  bool isIntersect = !((node.m_pts[2] <= r.minX()) || (node.m_pts[0] >= r.maxX()) ||
                       (node.m_pts[3] <= r.minY()) || (node.m_pts[1] >= r.maxY()));
  if (isIntersect)
    m_intersections.push_back(&node);
}

} // namespace dp
