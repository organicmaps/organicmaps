#include "../base/SRC_FIRST.hpp"

#include "layout_element.hpp"
#include "screen.hpp"

namespace yg
{
  LayoutElement::LayoutElement(int groupID, m2::PointD const & pivot, EPosition pos)
    : m_groupID(groupID), m_pivot(pivot), m_pos(pos)
  {}

  int LayoutElement::groupID() const
  {
    return m_groupID;
  }

  m2::PointD const & LayoutElement::pivot() const
  {
    return m_pivot;
  }

  EPosition LayoutElement::position() const
  {
    return m_pos;
  }

  bool LayoutElement::isFreeElement() const
  {
    return m_isFreeElement;
  }

  void LayoutElement::setIsFreeElement(bool flag) const
  {
    m_isFreeElement = flag;
  }

  bool LayoutElement::isFrozen() const
  {
    return m_isFrozen;
  }

  void LayoutElement::setIsFrozen(bool flag) const
  {
    m_isFrozen = flag;
  }

  bool LayoutElement::doNeedRedraw() const
  {
    return m_doNeedRedraw;
  }

  void LayoutElement::setNeedRedraw(bool flag) const
  {
    m_doNeedRedraw = flag;
  }
}
