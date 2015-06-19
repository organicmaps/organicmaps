#include "graphics/overlay.hpp"
#include "graphics/overlay_renderer.hpp"

#include "base/logging.hpp"
#include "base/stl_add.hpp"

#include "std/bind.hpp"
#include "std/vector.hpp"


namespace graphics
{

OverlayStorage::OverlayStorage()
  : m_needClip(false)
{
}

OverlayStorage::OverlayStorage(const m2::RectD & clipRect)
  : m_clipRect(clipRect)
  , m_needClip(true)
{
}

size_t OverlayStorage::GetSize() const
{
  return m_elements.size();
}

void OverlayStorage::AddElement(const shared_ptr<OverlayElement> & elem)
{
  if (m_needClip == false)
  {
    m_elements.push_back(elem);
    return;
  }

  OverlayElement::RectsT rects;
  elem->GetMiniBoundRects(rects);

  for (size_t j = 0; j < rects.size(); ++j)
  {
    if (m_clipRect.IsIntersect(rects[j]))
    {
      m_elements.push_back(elem);
      break;
    }
  }
}

bool betterOverlayElement(shared_ptr<OverlayElement> const & l,
                          shared_ptr<OverlayElement> const & r)
{
  // "frozen" object shouldn't be popped out.
  if (r->isFrozen())
    return false;

  // for the composite elements, collected in OverlayRenderer to replace the part elements
  return l->priority() > r->priority();
}

m2::RectD const OverlayElementTraits::LimitRect(shared_ptr<OverlayElement> const & elem)
{
  return elem->GetBoundRect();
}

size_t Overlay::getElementsCount() const
{
  return m_tree.GetSize();
}

void Overlay::lock()
{
  m_mutex.Lock();
}

void Overlay::unlock()
{
  m_mutex.Unlock();
}

void Overlay::clear()
{
  m_tree.Clear();
}

struct DoPreciseSelectByRect
{
  m2::AnyRectD m_rect;
  list<shared_ptr<OverlayElement> > * m_elements;

  DoPreciseSelectByRect(m2::RectD const & rect,
                        list<shared_ptr<OverlayElement> > * elements)
    : m_rect(rect),
      m_elements(elements)
  {}

  void operator()(shared_ptr<OverlayElement> const & e)
  {
    OverlayElement::RectsT rects;
    e->GetMiniBoundRects(rects);

    for (size_t i = 0; i < rects.size(); ++i)
    {
      if (m_rect.IsIntersect(rects[i]))
      {
        m_elements->push_back(e);
        break;
      }
    }
  }
};

class DoPreciseIntersect
{
  shared_ptr<OverlayElement> m_oe;
  OverlayElement::RectsT m_rects;

  bool m_isIntersect;

public:
  DoPreciseIntersect(shared_ptr<OverlayElement> const & oe)
    : m_oe(oe), m_isIntersect(false)
  {
    m_oe->GetMiniBoundRects(m_rects);
  }

  m2::RectD GetSearchRect() const
  {
    m2::RectD rect;
    for (size_t i = 0; i < m_rects.size(); ++i)
      rect.Add(m_rects[i].GetGlobalRect());
    return rect;
  }

  void operator()(shared_ptr<OverlayElement> const & e)
  {
    if (m_isIntersect)
      return;

    if (m_oe->m_userInfo == e->m_userInfo)
      return;

    OverlayElement::RectsT rects;
    e->GetMiniBoundRects(rects);

    for (size_t i = 0; i < m_rects.size(); ++i)
      for (size_t j = 0; j < rects.size(); ++j)
      {
        m_isIntersect = m_rects[i].IsIntersect(rects[j]);
        if (m_isIntersect)
          return;
      }
  }

  bool IsIntersect() const { return m_isIntersect; }
};

void Overlay::selectOverlayElements(m2::RectD const & rect, list<shared_ptr<OverlayElement> > & res) const
{
  DoPreciseSelectByRect fn(rect, &res);
  m_tree.ForEachInRect(rect, fn);
}

void Overlay::replaceOverlayElement(shared_ptr<OverlayElement> const & oe)
{
  DoPreciseIntersect fn(oe);
  m_tree.ForEachInRect(fn.GetSearchRect(), ref(fn));

  if (fn.IsIntersect())
    m_tree.ReplaceAllInRect(oe, &betterOverlayElement);
  else
    m_tree.Add(oe);
}

void Overlay::processOverlayElement(shared_ptr<OverlayElement> const & oe, math::Matrix<double, 3, 3> const & m)
{
  oe->setTransformation(m);
  if (oe->isValid())
    processOverlayElement(oe);
}

void Overlay::processOverlayElement(shared_ptr<OverlayElement> const & oe)
{
  if (oe->isValid())
    replaceOverlayElement(oe);
}

bool greater_priority(shared_ptr<OverlayElement> const & l,
                      shared_ptr<OverlayElement> const & r)
{
  return l->priority() > r->priority();
}

void Overlay::merge(shared_ptr<OverlayStorage> const & layer, math::Matrix<double, 3, 3> const & m)
{
  buffer_vector<shared_ptr<OverlayElement>, 256> v;
  v.reserve(layer->GetSize());

  // 1. collecting all elements from tree
  layer->ForEach(MakeBackInsertFunctor(v));

  // 2. sorting by priority, so the more important ones comes first
  sort(v.begin(), v.end(), &greater_priority);

  // 3. merging them into the infoLayer starting from most
  // important one to optimize the space usage.
  for_each(v.begin(), v.end(), [&] (shared_ptr<OverlayElement> const & p)
  {
    processOverlayElement(p, m);
  });
}

void Overlay::merge(shared_ptr<OverlayStorage> const & infoLayer)
{
  buffer_vector<shared_ptr<OverlayElement>, 265> v;
  v.reserve(infoLayer->GetSize());

  // 1. collecting all elements from tree
  infoLayer->ForEach(MakeBackInsertFunctor(v));

  // 2. sorting by priority, so the more important ones comes first
  sort(v.begin(), v.end(), &greater_priority);

  // 3. merging them into the infoLayer starting from most
  // important one to optimize the space usage.
  for_each(v.begin(), v.end(), [this] (shared_ptr<OverlayElement> const & p)
  {
    processOverlayElement(p);
  });
}

void Overlay::clip(m2::RectI const & r)
{
  vector<shared_ptr<OverlayElement> > v;
  v.reserve(m_tree.GetSize());
  m_tree.ForEach(MakeBackInsertFunctor(v));
  m_tree.Clear();

  m2::RectD const rd(r);
  m2::AnyRectD ard(rd);

  for (shared_ptr<OverlayElement> const & e : v)
  {
    if (!e->isVisible())
      continue;

    OverlayElement::RectsT rects;
    e->GetMiniBoundRects(rects);

    for (size_t j = 0; j < rects.size(); ++j)
      if (ard.IsIntersect(rects[j]))
      {
        processOverlayElement(e);
        break;
      }
  }
}

}
