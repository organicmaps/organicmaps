#pragma once

#include "point2d.hpp"
#include "rect2d.hpp"

#include "../base/math.hpp"

#include "../base/start_mem_debug.hpp"

enum EOrientation
{
  EOrientation0,
  EOrientation90,
  EOrientation180,
  EOrientation270
};

class ScreenBase
{
  m2::RectD m_PixelRect;
  m2::RectD m_GlobalRect;
  double m_Scale;
  double m_Angle;

protected:

  /// @group Dependent parameters
  /// Global to Pixel conversion matrix.
  ///             |a11 a12 0|
  /// |x, y, 1| * |a21 a22 0| = |x', y', 1|
  ///             |a31 a32 1|
  /// @{
  math::Matrix<double, 3, 3> m_GtoP;

  /// Pixel to Global conversion matrix. GtoP inverted
  math::Matrix<double, 3, 3> m_PtoG;

  /// X-axis aligned global rect used for clipping
  m2::RectD m_ClipRect;

  /// @}

  // Update dependent parameters from base parameters.
  // Must be called when base parameters changed.
  void UpdateDependentParameters();

public:

  ScreenBase();

  void SetFromRect(m2::RectD const & rect);
  void SetOrg(m2::PointD const & p);

  void Move(double dx, double dy);
  void MoveG(m2::PointD const & delta);
  void Scale(double scale);
  void Rotate(double angle);
  void ReverseTransformInPixelCoords(double s, double a, double dx, double dy);

  void OnSize(m2::RectI const & r)
  {
    m_PixelRect = m2::RectD(r);
    UpdateDependentParameters();
  }

  void OnSize(int x0, int y0, int w, int h)
  {
    m_PixelRect = m2::RectD(x0, y0, x0 + w, y0 + h);
    UpdateDependentParameters();
  }

public:

  inline double GetScale() const { return m_Scale; };

  inline double GetAngle() const
  {
    return m_Angle;
  }

  inline int GetWidth() const
  {
    return my::rounds(m_PixelRect.SizeX());
  }

  inline int GetHeight() const
  {
    return my::rounds(m_PixelRect.SizeY());
  }

  /// @warning ClipRect() returns a PtoG(m_PixelRect) rect.
  inline m2::RectD const & ClipRect() const
  {
    return m_ClipRect;
  }

  inline m2::PointD GtoP(m2::PointD const & pt) const
  {
    return pt * m_GtoP;
  }

  inline void GtoP(double & x, double & y) const
  {
    double tempX = x;
    x = tempX * m_GtoP(0, 0) + y * m_GtoP(1, 0) + m_GtoP(2, 0);
    y = tempX * m_GtoP(1, 0) + y * m_GtoP(1, 1) + m_GtoP(2, 1);
  }

  void GtoP(m2::RectD const & gr, m2::RectD & sr) const;
  void PtoG(m2::RectD const & sr, m2::RectD & gr) const;

  math::Matrix<double, 3, 3> const GtoPMatrix() const
  {
    return m_GtoP;
  }

  math::Matrix<double, 3, 3> const PtoGMatrix() const
  {
    return m_PtoG;
  }

  m2::RectD const & PixelRect() const
  {
    return m_PixelRect;
  }

  m2::RectD const & GlobalRect() const
  {
    return m_GlobalRect;
  }

  /// Compute arbitrary pixel transformation, that translates the (oldPt1, oldPt2) -> (newPt1, newPt2)
  static math::Matrix<double, 3, 3> const CalcTransform(m2::PointD const & oldPt1, m2::PointD const & oldPt2,
                                                        m2::PointD const & newPt1, m2::PointD const & newPt2);

  /// Setting GtoP matrix extracts the Angle and GlobalRect, leaving PixelRect intact
  void SetGtoPMatrix(math::Matrix<double, 3, 3> const & m);

  /// Extracting parameters from matrix, that is supposed to represent GtoP transformation
  static void ExtractGtoPParams(math::Matrix<double, 3, 3> const & m, double & a, double & s, double & dx, double & dy);

  inline m2::PointD PtoG(m2::PointD const & pt) const
  {
    return pt * m_PtoG;
  }

  inline void PtoG(double & x, double & y) const
  {
    double tempX = x;
    x = tempX * m_PtoG(0, 0) + y * m_PtoG(1, 0) + m_PtoG(2, 0);
    y = tempX * m_PtoG(0, 1) + y * m_PtoG(1, 1) + m_PtoG(2, 1);
  }

  bool operator != (ScreenBase const & src) const
  {
    return !(*this == src);
  }

  bool operator == (ScreenBase const & src) const
  {
    return (m_GtoP == src.m_GtoP) && (m_PtoG == src.m_PtoG);
  }
};

bool IsPanning(ScreenBase const & s1, ScreenBase const & s2);

#include "../base/stop_mem_debug.hpp"
