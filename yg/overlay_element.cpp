#include "../base/SRC_FIRST.hpp"
#include "overlay_element.hpp"

namespace yg
{
  OverlayElement::~OverlayElement()
  {}

  OverlayElement::Params::Params()
    : m_pivot(), m_position(yg::EPosAboveRight), m_depth(yg::maxDepth)
  {}

  OverlayElement::OverlayElement(Params const & p)
    : m_pivot(p.m_pivot),
      m_position(p.m_position),
      m_depth(p.m_depth),
      m_isNeedRedraw(true),
      m_isFrozen(false),
      m_isVisible(true),
      m_isValid(true),
      m_isDirtyRect(true),
      m_isDirtyRoughRect(true)
  {}

  m2::PointD const OverlayElement::tieRect(m2::RectD const & r, math::Matrix<double, 3, 3> const & m) const
  {
    m2::PointD res;

    yg::EPosition pos = position();
    m2::PointD pt = pivot() * m;

    if (pos & EPosLeft)
      res.x = pt.x - r.SizeX();
    else if (pos & EPosRight)
      res.x = pt.x;
    else
      res.x = pt.x - r.SizeX() / 2;

    if (pos & EPosAbove)
      res.y = pt.y - r.SizeY();
    else if (pos & EPosUnder)
      res.y = pt.y;
    else
      res.y = pt.y - r.SizeY() / 2;

    return res;
  }

  void OverlayElement::offset(m2::PointD const & offs)
  {
    setPivot(pivot() + offs);
    setIsDirtyRect(true);
  }

  m2::PointD const & OverlayElement::pivot() const
  {
    return m_pivot;
  }

  void OverlayElement::setPivot(m2::PointD const & pivot)
  {
    m_pivot = pivot;
    setIsDirtyRect(true);
  }

  yg::EPosition OverlayElement::position() const
  {
    return m_position;
  }

  void OverlayElement::setPosition(yg::EPosition pos)
  {
    m_position = pos;
    setIsDirtyRect(true);
  }

  double OverlayElement::depth() const
  {
    return m_depth;
  }

  void OverlayElement::setDepth(double depth)
  {
    m_depth = depth;
  }

  bool OverlayElement::isFrozen() const
  {
    return m_isFrozen;
  }

  void OverlayElement::setIsFrozen(bool flag)
  {
    m_isFrozen = flag;
  }

  bool OverlayElement::isNeedRedraw() const
  {
    return m_isNeedRedraw;
  }

  void OverlayElement::setIsNeedRedraw(bool flag)
  {
    m_isNeedRedraw = flag;
  }

  bool OverlayElement::isVisible() const
  {
    return m_isVisible;
  }

  void OverlayElement::setIsVisible(bool flag)
  {
    m_isVisible = flag;
  }

  bool OverlayElement::isDirtyRect() const
  {
    return m_isDirtyRect;
  }

  void OverlayElement::setIsDirtyRect(bool flag) const
  {
    if (flag)
      m_isDirtyRoughRect = true;
    m_isDirtyRect = flag;
  }

  m2::RectD const & OverlayElement::roughBoundRect() const
  {
    if (m_isDirtyRoughRect)
    {
      for (int i = 0; i < boundRects().size(); ++i)
        if (i == 0)
          m_roughBoundRect = boundRects()[i].GetGlobalRect();
        else
          m_roughBoundRect.Add(boundRects()[i].GetGlobalRect());

      m_isDirtyRoughRect = false;
    }
    return m_roughBoundRect;
  }

  bool OverlayElement::hitTest(m2::PointD const & pt) const
  {
    vector<m2::AnyRectD> const & rects = boundRects();

    for (vector<m2::AnyRectD>::const_iterator it = rects.begin(); it != rects.end(); ++it)
      if (it->IsPointInside(pt))
        return true;

    return false;
  }

  bool OverlayElement::isValid() const
  {
    return m_isValid;
  }

  void OverlayElement::setIsValid(bool flag)
  {
    m_isValid = flag;
  }

  bool OverlayElement::roughHitTest(m2::PointD const & pt) const
  {
    return roughBoundRect().IsPointInside(pt);
  }
}
