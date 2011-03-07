#include "screenbase.hpp"

#include "../std/cmath.hpp"
#include "../base/matrix.hpp"
#include "../base/logging.hpp"
#include "transformations.hpp"
#include "angles.hpp"

#include "../base/start_mem_debug.hpp"

ScreenBase::ScreenBase() :
    m_PixelRect(0, 0, 640, 480),
    m_GlobalRect(0, 0, 640, 480),
    m_Scale(1),
    m_Angle(0.0)
{
  m_GtoP = math::Identity<double, 3>();
  m_PtoG = math::Identity<double, 3>();
//  UpdateDependentParameters();
}

void ScreenBase::UpdateDependentParameters()
{
  m_PtoG = math::Shift( /// 5. shifting on (E0, N0)
               math::Rotate( /// 4. rotating on the screen angle
                   math::Scale( /// 3. scaling to translate pixel sizes to global
                       math::Scale( /// 2. swapping the Y axis??? why??? supposed to be a rotation on -pi / 2 here.
                           math::Shift( /// 1. shifting for the pixel center to become (0, 0)
                               math::Identity<double, 3>(),
                               - m_PixelRect.Center()
                               ),
                           1,
                           -1
                           ),
                       m_Scale,
                       m_Scale
                       ),
                   GetAngle()
               ),
               m_GlobalRect.Center()
           );

  m_GtoP = math::Inverse(m_PtoG);

  PtoG(m_PixelRect, m_ClipRect);
}

void ScreenBase::SetFromRect(m2::RectD const & GlobalRect)
{
  double hScale = GlobalRect.SizeX() / m_PixelRect.SizeX();
  double vScale = GlobalRect.SizeY() / m_PixelRect.SizeY();

  m_Scale = max(hScale, vScale);
  m_Angle = 0;

  /// Fit the global rect into pixel rect

  m2::PointD HalfSize(m_Scale * m_PixelRect.SizeX() / 2,
                      m_Scale * m_PixelRect.SizeY() / 2);

  m_GlobalRect = m2::RectD(
      GlobalRect.Center() - HalfSize,
      GlobalRect.Center() + HalfSize
      );

  UpdateDependentParameters();
}

void ScreenBase::SetOrg(m2::PointD const & p)
{
  m_GlobalRect.Offset(p - m_GlobalRect.Center());
  UpdateDependentParameters();
}

void ScreenBase::Move(double dx, double dy)
{
  m_GlobalRect.Offset(m_GlobalRect.Center() - PtoG(m_PixelRect.Center() + m2::PointD(dx, dy)));

  UpdateDependentParameters();
}

void ScreenBase::MoveG(m2::PointD const & delta)
{
  m_GlobalRect.Offset(delta);

  UpdateDependentParameters();
}

void ScreenBase::Scale(double scale)
{
  m_GlobalRect.Scale( 1 / scale);
  m_Scale /= scale;
  UpdateDependentParameters();
}

void ScreenBase::Rotate(double angle)
{
  m_Angle -= angle;
  UpdateDependentParameters();
}

math::Matrix<double, 3, 3> const ScreenBase::CalcTransform(m2::PointD const & oldPt1, m2::PointD const & oldPt2,
                                                           m2::PointD const & newPt1, m2::PointD const & newPt2)
{
  double s = newPt1.Length(newPt2) / oldPt1.Length(oldPt2);
  double a = ang::AngleTo(newPt1, newPt2) - ang::AngleTo(oldPt1, oldPt2);

  math::Matrix<double, 3, 3> m =
      math::Shift(
          math::Scale(
              math::Rotate(
                  math::Shift(
                      math::Identity<double, 3>(),
                      -oldPt1.x, -oldPt1.y
                      ),
                  a
                  ),
              s, s
              ),
          newPt1.x, newPt1.y
          );
  return m;
}

void ScreenBase::SetGtoPMatrix(math::Matrix<double, 3, 3> const & m)
{
  m_GtoP = m;
  m_PtoG = math::Inverse(m_GtoP);
  /// Extracting transformation params, assuming that the matrix
  /// somehow represent a valid screen transformation
  /// into m_PixelRectangle
  double dx, dy;
  ExtractGtoPParams(m, m_Angle, m_Scale, dx, dy);
  m_Scale = 1 / m_Scale;

  m_GlobalRect = m_PixelRect;
  m_GlobalRect.Scale(m_Scale);

  m_GlobalRect.Offset(m_PixelRect.Center() * m_PtoG - m_GlobalRect.Center());
}

void ScreenBase::GtoP(m2::RectD const & gr, m2::RectD & sr) const
{
  sr = m2::RectD(GtoP(gr.LeftTop()), GtoP(gr.RightBottom()));
}

void ScreenBase::PtoG(m2::RectD const & sr, m2::RectD & gr) const
{
  gr = m2::RectD(PtoG(sr.LeftTop()), PtoG(sr.RightBottom()));
}

bool IsPanning(ScreenBase const & s1, ScreenBase const & s2)
{
  m2::PointD globPt(s1.GlobalRect().Center().x - s1.GlobalRect().minX(),
                    s1.GlobalRect().Center().y - s1.GlobalRect().minY());

  m2::PointD p1 = s1.GtoP(s1.GlobalRect().Center()) - s1.GtoP(s1.GlobalRect().Center() + globPt);

  m2::PointD p2 = s2.GtoP(s2.GlobalRect().Center()) - s2.GtoP(s2.GlobalRect().Center() + globPt);

  return p1.EqualDxDy(p2, 0.00001);
}

void ScreenBase::ExtractGtoPParams(math::Matrix<double, 3, 3> const & m, double &a, double &s, double &dx, double &dy)
{
  s = sqrt(m(0, 0) * m(0, 0) + m(0, 1) * m(0, 1));
  double cosA = m(0, 0) / s;
  double sinA = -m(0, 1) / s;

  if (cosA != 0)
    a = atan2(sinA, cosA);
  else
    if (sinA > 0)
      a = math::pi / 2;
    else
      a = 3 * math::pi / 2;

  dx = m(2, 0);
  dy = m(2, 1);
}

