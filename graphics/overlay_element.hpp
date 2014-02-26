#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/any_rect2d.hpp"
#include "../base/matrix.hpp"
#include "defines.hpp"
#include "../std/vector.hpp"

namespace graphics
{
  class OverlayRenderer;

  class OverlayElement
  {
  public:

    struct UserInfo
    {
      size_t m_mwmID;
      uint32_t m_offset;

      UserInfo() : m_mwmID(size_t(-1)) {}
      inline bool IsValid() const { return (m_mwmID != size_t(-1)); }
      inline bool operator== (UserInfo const & a) const
      {
        return IsValid() && (a.m_mwmID == m_mwmID) && (a.m_offset == m_offset);
      }
    };

  private:

    m2::PointD m_pivot;
    graphics::EPosition m_position;
    double m_depth;    

    bool m_isNeedRedraw;
    bool m_isFrozen;
    bool m_isVisible;
    bool m_isValid;
    mutable bool m_isDirtyRect;
    mutable bool m_isDirtyLayout;

    mutable bool m_isDirtyRoughRect;
    mutable m2::RectD m_roughBoundRect;

    math::Matrix<double, 3, 3> m_inverseMatrix;

  protected:
    math::Matrix<double, 3, 3> const & getResetMatrix() const;

  public:

    UserInfo m_userInfo;
    static m2::PointD const computeTopLeft(m2::PointD const & sz,
                                           m2::PointD const & pv,
                                           EPosition pos);

    struct Params
    {
      m2::PointD m_pivot;
      graphics::EPosition m_position;
      double m_depth;
      UserInfo m_userInfo;
      Params();
    };

    OverlayElement(Params const & p);
    virtual ~OverlayElement();

    /// PLEASE, REMEMBER THE REFERENCE!!!
    virtual vector<m2::AnyRectD> const & boundRects() const = 0;
    virtual void draw(OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const = 0;
    /// Set new transformation ! RELATIVE TO INIT STATE ! for drawing and safe information for reseting
    /// Need to call base class method
    virtual void setTransformation(math::Matrix<double, 3, 3> const & m) = 0;
    /// This method reset transformation to initial state.
    /// Geometry stored in coordinates relative to the tile.
    /// Need to call base class method
    virtual void resetTransformation();

    virtual double priority() const;

    m2::PointD const & pivot() const;
    virtual void setPivot(m2::PointD const & pv);

    m2::PointD const point(EPosition pos) const;

    void offset(m2::PointD const & offs);

    graphics::EPosition position() const;
    void setPosition(graphics::EPosition pos);

    double depth() const;
    void setDepth(double depth);

    bool isFrozen() const;
    void setIsFrozen(bool flag);

    bool isNeedRedraw() const;
    void setIsNeedRedraw(bool flag);

    bool isDirtyRect() const;
    void setIsDirtyRect(bool flag) const;

    bool isDirtyLayout() const;
    void setIsDirtyLayout(bool flag) const;

    bool isVisible() const;
    virtual void setIsVisible(bool flag);

    bool isValid() const;
    void setIsValid(bool flag);

    m2::PointD getOffset() const;
    void setOffset(m2::PointD offset);

    UserInfo const & userInfo() const;

    virtual bool hitTest(m2::PointD const & pt) const;
    virtual bool roughHitTest(m2::PointD const & pt) const;

    m2::RectD const & roughBoundRect() const;

    virtual bool hasSharpGeometry() const;
  };

}
