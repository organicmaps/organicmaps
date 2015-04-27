#pragma once

#include "graphics/defines.hpp"
#include "graphics/color.hpp"

#include "indexer/mwm_set.hpp"

#include "geometry/point2d.hpp"
#include "geometry/any_rect2d.hpp"

#include "base/matrix.hpp"
#include "base/buffer_vector.hpp"

#include "std/bitset.hpp"


namespace graphics
{
  class OverlayRenderer;

  class OverlayElement
  {
  public:
    struct UserInfo
    {
      MwmSet::MwmId m_mwmID;
      uint32_t m_offset;

      UserInfo() = default;
      inline bool IsValid() const { return m_mwmID.IsAlive(); }
      inline bool operator== (UserInfo const & a) const
      {
        return IsValid() && (a.m_mwmID == m_mwmID) && (a.m_offset == m_offset);
      }
    };

  private:
    m2::PointD m_pivot;
    graphics::EPosition m_position;
    double m_depth;    

    enum { NEED_REDRAW,
           FROZEN,
           VISIBLE,
           VALID,
           DIRTY_LAYOUT,
           FLAGS_COUNT };

    mutable bitset<FLAGS_COUNT> m_flags;

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

    /// @name Getting element boundaries.
    //@{
    virtual m2::RectD GetBoundRect() const;
    typedef buffer_vector<m2::AnyRectD, 4> RectsT;
    virtual void GetMiniBoundRects(RectsT & rects) const;
    //@}

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
    virtual void setPivot(m2::PointD const & pv, bool dirtyFlag = true);

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

    bool isDirtyLayout() const;
    virtual void setIsDirtyLayout(bool flag) const;

    virtual bool isVisible() const;
    virtual void setIsVisible(bool flag) const;

    bool isValid() const;
    void setIsValid(bool flag);

    m2::PointD getOffset() const;
    void setOffset(m2::PointD offset);

    UserInfo const & userInfo() const;

    virtual bool hitTest(m2::PointD const & pt) const;
    virtual bool hasSharpGeometry() const;

    void DrawRectsDebug(graphics::OverlayRenderer * r, Color color, double depth) const;
  };
}
