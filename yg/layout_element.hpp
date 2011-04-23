#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"
#include "defines.hpp"

namespace yg
{
  class Screen;
  struct FontDesc;

  class LayoutElement
  {
  private:

    int m_groupID;
    m2::PointD m_pivot;
    EPosition m_pos;

    mutable bool m_isFreeElement;
    mutable bool m_isFrozen;
    mutable bool m_doNeedRedraw;

  public:
    LayoutElement(int groupID, m2::PointD const & pivot, EPosition pos);
    /// id of the group, composed of several layoutElements
    int groupID() const;
    /// pivot is expressed in group coordinates
    m2::PointD const & pivot() const;
    /// position of the element related to pivot point
    EPosition position() const;

    bool isFreeElement() const;
    void setIsFreeElement(bool flag) const;

    bool isFrozen() const;
    void setIsFrozen(bool flag) const;

    bool doNeedRedraw() const;
    void setNeedRedraw(bool flag) const;

    /// bounding rect in pivot-aligned coordinates
    virtual m2::RectD const boundRect() const = 0;
    /// draw layout element
    virtual void draw(Screen * screen) = 0;
  };

  class TextLayoutElement : public LayoutElement
  {
  public:

    TextLayoutElement(char const * text, double depth, FontDesc const & fontDesc);

    m2::RectD const boundRect() const;

    void draw(Screen * screen);
  };

  class SymbolLayoutElement : public LayoutElement
  {
  public:

    SymbolLayoutElement();

    m2::RectD const boundRect() const;

    void draw(Screen * screen);
  };
}
