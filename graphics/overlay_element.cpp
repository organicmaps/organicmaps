#include "overlay_element.hpp"


namespace graphics
{
  OverlayElement::~OverlayElement()
  {}

  OverlayElement::Params::Params()
    : m_pivot(0, 0),
      m_position(EPosAboveRight),
      m_depth(0),
      m_userInfo()
  {}

  OverlayElement::OverlayElement(Params const & p)
    : m_pivot(p.m_pivot),
      m_position(p.m_position),
      m_depth(p.m_depth),
      m_inverseMatrix(math::Identity<double, 3>()),
      m_userInfo(p.m_userInfo)
  {
    m_flags.set();
    m_flags[FROZEN] = false;
  }

  m2::PointD const OverlayElement::computeTopLeft(m2::PointD const & sz,
                                                  m2::PointD const & pv,
                                                  EPosition pos)
  {
    m2::PointD res;

    if (pos & EPosLeft)
      res.x = pv.x - sz.x;
    else if (pos & EPosRight)
      res.x = pv.x;
    else
      res.x = pv.x - sz.x / 2;

    if (pos & EPosAbove)
      res.y = pv.y - sz.y;
    else if (pos & EPosUnder)
      res.y = pv.y;
    else
      res.y = pv.y - sz.y / 2;

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

  graphics::EPosition OverlayElement::position() const
  {
    return m_position;
  }

  void OverlayElement::setPosition(graphics::EPosition pos)
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
    return m_flags[FROZEN];
  }

  void OverlayElement::setIsFrozen(bool flag)
  {
    m_flags[FROZEN] = flag;
  }

  bool OverlayElement::isNeedRedraw() const
  {
    return m_flags[NEED_REDRAW];
  }

  void OverlayElement::setIsNeedRedraw(bool flag)
  {
    m_flags[NEED_REDRAW] = flag;
  }

  bool OverlayElement::isVisible() const
  {
    return m_flags[VISIBLE];
  }

  void OverlayElement::setIsVisible(bool flag)
  {
    m_flags[VISIBLE] = flag;
  }

  bool OverlayElement::isDirtyLayout() const
  {
   return m_flags[DIRTY_LAYOUT];
  }

  void OverlayElement::setIsDirtyLayout(bool flag) const
  {
    m_flags[DIRTY_LAYOUT] = flag;
    if (flag)
      setIsDirtyRect(true);
  }

  bool OverlayElement::isDirtyRect() const
  {
    return m_flags[DIRTY_RECT];
  }

  void OverlayElement::setIsDirtyRect(bool flag) const
  {
    if (flag)
      m_flags[DIRTY_ROUGH_RECT] = true;
    m_flags[DIRTY_RECT] = flag;
  }

  m2::RectD const & OverlayElement::roughBoundRect() const
  {
    if (m_flags[DIRTY_ROUGH_RECT])
    {
      vector<m2::AnyRectD> const & rects = boundRects();
      size_t const count = rects.size();
      if (count == 0)
      {
        /// @todo Is it correct use-case?
        m_roughBoundRect = m2::RectD(pivot(), pivot());
      }
      else
      {
        m_roughBoundRect = rects[0].GetGlobalRect();
        for (size_t i = 1; i < count; ++i)
          m_roughBoundRect.Add(rects[i].GetGlobalRect());
      }

      m_flags[DIRTY_ROUGH_RECT] = false;
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
    return m_flags[VALID];
  }

  void OverlayElement::setIsValid(bool flag)
  {
    m_flags[VALID] = flag;
  }

  bool OverlayElement::roughHitTest(m2::PointD const & pt) const
  {
    return roughBoundRect().IsPointInside(pt);
  }

  OverlayElement::UserInfo const & OverlayElement::userInfo() const
  {
    return m_userInfo;
  }

  m2::PointD const OverlayElement::point(EPosition pos) const
  {
    /// @todo It's better to call roughBoundRect(), or place ASSERT(!m_isDirtyRoughRect, ()) here.
    /// In general there is no need to store m_roughBoundRect at all.
    /// It's calculating time is fast, because elements already cache vector<m2::AnyRectD>.

    m2::PointD res = m_roughBoundRect.Center();

    if (pos & EPosLeft)
      res.x = m_roughBoundRect.minX();
    if (pos & EPosRight)
      res.x = m_roughBoundRect.maxX();

    if (pos & EPosAbove)
      res.y = m_roughBoundRect.minY();
    if (pos & EPosUnder)
      res.y = m_roughBoundRect.maxY();

    return res;
  }


  bool OverlayElement::hasSharpGeometry() const
  {
    return false;
  }

  double OverlayElement::priority() const
  {
    return m_depth;
  }

  math::Matrix<double, 3, 3> const & OverlayElement::getResetMatrix() const
  {
    return m_inverseMatrix;
  }

  void OverlayElement::setTransformation(const math::Matrix<double, 3, 3> & m)
  {
    m_inverseMatrix = math::Inverse(m);
  }

  void OverlayElement::resetTransformation()
  {
    setTransformation(math::Identity<double, 3>());
  }
}
