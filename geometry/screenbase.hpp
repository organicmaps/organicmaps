#pragma once

#include "point2d.hpp"
#include "rect2d.hpp"
#include "any_rect2d.hpp"

#include "../base/math.hpp"

class ScreenBase
{
  m2::RectD m_PixelRect;

  double m_Scale;
  ang::AngleD m_Angle;
  m2::PointD m_Org;

protected:

  /// @group Dependent parameters
  /// Global to Pixel conversion matrix.
  ///             |a11 a12 0|
  /// |x, y, 1| * |a21 a22 0| = |x', y', 1|
  ///             |a31 a32 1|
  /// @{
  math::Matrix<double, 3, 3> m_GtoP;

  /// Pixel to Global conversion matrix. Inverted GtoP matrix.
  math::Matrix<double, 3, 3> m_PtoG;

  /// Global Rect
  m2::AnyRectD m_GlobalRect;

  /// X-axis aligned global rect used for clipping
  m2::RectD m_ClipRect;

  /// @}

  // Update dependent parameters from base parameters.
  // Must be called when base parameters changed.
  void UpdateDependentParameters();

public:

  ScreenBase();
  ScreenBase(m2::RectI const & pxRect, m2::AnyRectD const & glbRect);

  void SetFromRect(m2::AnyRectD const & rect);
  void SetFromRects(m2::AnyRectD const & glbRect, m2::RectD const & pxRect);
  void SetOrg(m2::PointD const & p);

  void Move(double dx, double dy);
  void MoveG(m2::PointD const & p);

  /// scale global rect
  void Scale(double scale);
  void Rotate(double angle);

  void OnSize(m2::RectI const & r);
  void OnSize(int x0, int y0, int w, int h);

public:

  double GetScale() const;

  double GetAngle() const;
  void SetAngle(double angle);

  m2::PointD const & GetOrg() const;

  int GetWidth() const;
  int GetHeight() const;

  inline m2::PointD GtoP(m2::PointD const & pt) const
  {
    return pt * m_GtoP;
  }

  inline m2::PointD PtoG(m2::PointD const & pt) const
  {
    return pt * m_PtoG;
  }

  inline void GtoP(double & x, double & y) const
  {
    double tempX = x;
    x = tempX * m_GtoP(0, 0) + y * m_GtoP(1, 0) + m_GtoP(2, 0);
    y = tempX * m_GtoP(1, 0) + y * m_GtoP(1, 1) + m_GtoP(2, 1);
  }

  inline void PtoG(double & x, double & y) const
  {
    double tempX = x;
    x = tempX * m_PtoG(0, 0) + y * m_PtoG(1, 0) + m_PtoG(2, 0);
    y = tempX * m_PtoG(0, 1) + y * m_PtoG(1, 1) + m_PtoG(2, 1);
  }

  void GtoP(m2::RectD const & gr, m2::RectD & sr) const;
  void PtoG(m2::RectD const & pr, m2::RectD & gr) const;

  void GetTouchRect(m2::PointD const & pixPoint, double pixRadius, m2::AnyRectD & glbRect) const;

  math::Matrix<double, 3, 3> const & GtoPMatrix() const;
  math::Matrix<double, 3, 3> const & PtoGMatrix() const;

  m2::RectD const & PixelRect() const;
  m2::AnyRectD const & GlobalRect() const;
  m2::RectD const & ClipRect() const;

  double GetMinPixelRectSize() const;

  /// Compute arbitrary pixel transformation, that translates the (oldPt1, oldPt2) -> (newPt1, newPt2)
  static math::Matrix<double, 3, 3> const CalcTransform(m2::PointD const & oldPt1, m2::PointD const & oldPt2,
                                                        m2::PointD const & newPt1, m2::PointD const & newPt2);

  /// Setting GtoP matrix extracts the Angle and m_Org parameters, leaving PixelRect intact
  void SetGtoPMatrix(math::Matrix<double, 3, 3> const & m);

  /// Extracting parameters from matrix, that is supposed to represent GtoP transformation
  static void ExtractGtoPParams(math::Matrix<double, 3, 3> const & m, double & a, double & s, double & dx, double & dy);

  bool operator != (ScreenBase const & src) const
  {
    return !(*this == src);
  }

  bool operator == (ScreenBase const & src) const
  {
    return (m_GtoP == src.m_GtoP) && (m_PtoG == src.m_PtoG);
  }
};

/// checking whether the s1 transforms into s2 without scaling, only with shift and rotation
bool IsPanningAndRotate(ScreenBase const & s1, ScreenBase const & s2);
