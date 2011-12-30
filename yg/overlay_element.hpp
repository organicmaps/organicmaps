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

  class StylesCache;

  class OverlayElement
  {
  private:

    m2::PointD m_pivot;
    yg::EPosition m_position;
    double m_depth;

    bool m_isNeedRedraw;
    bool m_isFrozen;
    bool m_isVisible;
    mutable bool m_isDirtyRect;

    mutable bool m_isDirtyRoughRect;
    mutable m2::RectD m_roughBoundRect;

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
    virtual ~OverlayElement();

    virtual OverlayElement * clone(math::Matrix<double, 3, 3> const & m) const = 0;

    /// PLEASE, REMEMBER THE REFERENCE!!!
    virtual vector<m2::AnyRectD> const & boundRects() const = 0;
    virtual void draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const = 0;
    virtual int visualRank() const = 0;

    /// caching-related functions.
    /// @{
    virtual void getNonPackedRects(StylesCache * stylesCache, vector<m2::PointU> & sizes) const = 0;
    virtual bool find(StylesCache * stylesCache) const = 0;
    virtual void map(StylesCache * stylesCache) const = 0;
    /// @}

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

    bool isDirtyRect() const;
    void setIsDirtyRect(bool flag) const;

    bool isVisible() const;
    void setIsVisible(bool flag);

    m2::RectD const & roughBoundRect() const;

    virtual void offset(m2::PointD const & offs);
  };

}
