#include "../base/SRC_FIRST.hpp"
#include "overlay_element.hpp"

namespace yg
{
  OverlayElement::Params::Params()
    : m_pivot(), m_position(yg::EPosAboveRight), m_depth(yg::maxDepth)
  {}

  OverlayElement::OverlayElement(Params const & p)
    : m_pivot(p.m_pivot),
      m_position(p.m_position),
      m_depth(p.m_depth),
      m_isNeedRedraw(true),
      m_isFrozen(false)
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
    m_pivot += offs;
  }

  m2::PointD const & OverlayElement::pivot() const
  {
    return m_pivot;
  }

  void OverlayElement::setPivot(m2::PointD const & pivot)
  {
    m_pivot = pivot;
  }

  yg::EPosition OverlayElement::position() const
  {
    return m_position;
  }

  void OverlayElement::setPosition(yg::EPosition pos)
  {
    m_position = pos;
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
}
