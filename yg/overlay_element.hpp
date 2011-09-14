#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/aa_rect2d.hpp"
#include "../base/matrix.hpp"
#include "defines.hpp"

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

  protected:

    m2::PointD const tieRect(m2::RectD const & r, math::Matrix<double, 3, 3> const & m) const;

  public:

    struct Params
    {
      m2::PointD m_pivot;
      yg::EPosition m_position;
      double m_depth;
      Params();
    };

    OverlayElement(Params const & p);

    virtual m2::AARectD const boundRect() const = 0;
    virtual void draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const = 0;

    m2::PointD const & pivot() const;
    void setPivot(m2::PointD const & pv);

    yg::EPosition position() const;
    void setPosition(yg::EPosition pos);

    double depth() const;
    void setDepth(double depth);

    bool isFrozen() const;
    void setIsFrozen(bool flag);

    bool isNeedRedraw() const;
    void setIsNeedRedraw(bool flag);

    void offset(m2::PointD const & offs);
  };

}
