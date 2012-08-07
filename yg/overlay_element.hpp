#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/any_rect2d.hpp"
#include "../base/matrix.hpp"
#include "defines.hpp"
#include "../std/vector.hpp"

namespace yg
{
  namespace gl
  {
    class OverlayRenderer;
  }

  class OverlayElement
  {
  private:

    m2::PointD m_pivot;
    yg::EPosition m_position;
    double m_depth;

    bool m_isNeedRedraw;
    bool m_isFrozen;
    bool m_isVisible;
    bool m_isValid;
    mutable bool m_isDirtyRect;
    mutable bool m_isDirtyDrawing;

    mutable bool m_isDirtyRoughRect;
    mutable m2::RectD m_roughBoundRect;

  public:

    m2::PointD const tieRect(m2::RectD const & r, math::Matrix<double, 3, 3> const & m) const;

    struct Params
    {
      m2::PointD m_pivot;
      yg::EPosition m_position;
      double m_depth;
      Params();
    };

    OverlayElement(Params const & p);
    virtual ~OverlayElement();

    virtual OverlayElement * clone(math::Matrix<double, 3, 3> const & m) const = 0;

    /// PLEASE, REMEMBER THE REFERENCE!!!
    virtual vector<m2::AnyRectD> const & boundRects() const = 0;
    virtual void draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const = 0;
    virtual int visualRank() const;

    m2::PointD const & pivot() const;
    virtual void setPivot(m2::PointD const & pv);

    void offset(m2::PointD const & offs);

    yg::EPosition position() const;
    void setPosition(yg::EPosition pos);

    double depth() const;
    void setDepth(double depth);

    bool isFrozen() const;
    void setIsFrozen(bool flag);

    bool isNeedRedraw() const;
    void setIsNeedRedraw(bool flag);

    bool isDirtyRect() const;
    void setIsDirtyRect(bool flag) const;

    bool isDirtyDrawing() const;
    void setIsDirtyDrawing(bool flag) const;

    bool isVisible() const;
    void setIsVisible(bool flag);

    bool isValid() const;
    void setIsValid(bool flag);

    bool hitTest(m2::PointD const & pt) const;
    bool roughHitTest(m2::PointD const & pt) const;

    m2::RectD const & roughBoundRect() const;
  };

}
