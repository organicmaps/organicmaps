#include "drape_frontend/navigator.hpp"
#include "drape_frontend/screen_operations.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/scales.hpp"

#include "geometry/angles.hpp"
#include "geometry/point2d.hpp"

namespace df
{
double const kDefault3dScale = 1.0;

void Navigator::SetFromScreen(ScreenBase const & screen)
{
  VisualParams const & p = VisualParams::Instance();
  SetFromScreen(screen, p.GetTileSize(), p.GetVisualScale());
}

void Navigator::SetFromScreen(ScreenBase const & screen, uint32_t tileSize, double visualScale)
{
  ScreenBase tmp = screen;
  if (!CheckMinScale(tmp) || !CheckBorders(tmp))
    ScaleInto(tmp, df::GetWorldRect());

  if (!CheckMaxScale(tmp, tileSize, visualScale))
  {
    int const scale = scales::GetUpperStyleScale() - 1;
    m2::RectD newRect = df::GetRectForDrawScale(scale, screen.GetOrg());
    newRect.Scale(m_Screen.GetScale3d());
    CheckMinMaxVisibleScale(newRect, scale, m_Screen.GetScale3d());
    tmp = m_Screen;
    tmp.SetFromRect(m2::AnyRectD(newRect));

    ASSERT(CheckMaxScale(tmp, tileSize, visualScale), ());
  }

  m_Screen = tmp;

  if (!m_InAction)
    m_StartScreen = tmp;
}

void Navigator::SetFromRect(m2::AnyRectD const & r)
{
  VisualParams const & p = VisualParams::Instance();
  SetFromRect(r, p.GetTileSize(), p.GetVisualScale());
}

void Navigator::SetFromRect(m2::AnyRectD const & r, uint32_t tileSize, double visualScale)
{
  ScreenBase tmp = m_Screen;
  tmp.SetFromRect(r);
  SetFromScreen(tmp, tileSize, visualScale);
}

void Navigator::CenterViewport(m2::PointD const & p)
{
  // Rounding center point to a pixel boundary to obtain crisp centered picture.
  m2::PointD pt = m_Screen.GtoP(p);
  pt.x = ceil(pt.x);
  pt.y = ceil(pt.y);

  pt = m_Screen.PtoG(pt);

  m_Screen.SetOrg(pt);
  if (!m_InAction)
    m_StartScreen.SetOrg(pt);
}

void Navigator::OnSize(int w, int h)
{
  m2::RectD const & worldR = df::GetWorldRect();

  m_Screen.OnSize(0, 0, w, h);
  ShrinkAndScaleInto(m_Screen, worldR);

  m_StartScreen.OnSize(0, 0, w, h);
  ShrinkAndScaleInto(m_StartScreen, worldR);
}

m2::PointD Navigator::GtoP(m2::PointD const & pt) const
{
  return m_Screen.GtoP(pt);
}

m2::PointD Navigator::PtoG(m2::PointD const & pt) const
{
  return m_Screen.PtoG(pt);
}

m2::PointD Navigator::P3dtoP(m2::PointD const & pt) const
{
  return m_Screen.P3dtoP(pt);
}

void Navigator::StartDrag(m2::PointD const & pt)
{
  m_StartPt1 = m_LastPt1 = pt;
  m_StartScreen = m_Screen;
  m_InAction = true;
}

void Navigator::DoDrag(m2::PointD const & pt)
{
  if (m_LastPt1 == pt)
    return;

  ShrinkInto(m_StartScreen, df::GetWorldRect());

  // Touch points are in P3d (viewport) space; Move() expects a 2D-pixel-rect delta.
  m2::PointD const startPt2d = m_StartScreen.P3dtoP(m_StartPt1);
  m2::PointD const endPt2d = m_StartScreen.P3dtoP(pt);
  double dx = endPt2d.x - startPt2d.x;
  double dy = endPt2d.y - startPt2d.y;

  // X is unconstrained (world wraps horizontally) — only test Y drag against pole boundaries.
  {
    ScreenBase tmp = m_StartScreen;
    tmp.Move(0, dy);
    if (!CheckBorders(tmp))
      dy = 0;
  }

  m_StartScreen.Move(dx, dy);
  // Clamp Y back within pole boundaries. Move() can introduce tiny floating-point drift in Y
  // even for horizontal-only drags, which would make CheckBorders fail at the exact Y boundary.
  ShrinkInto(m_StartScreen, df::GetWorldRect());

  m_StartPt1 = pt;
  m_LastPt1 = pt;
  m_Screen = m_StartScreen;
}

void Navigator::StopDrag(m2::PointD const & pt)
{
  DoDrag(pt);
  m_InAction = false;
}

bool Navigator::InAction() const
{
  return m_InAction;
}

void Navigator::StartScale(m2::PointD const & pt1, m2::PointD const & pt2)
{
  m_StartScreen = m_Screen;
  m_StartPt1 = m_LastPt1 = pt1;
  m_StartPt2 = m_LastPt2 = pt2;

  m_DoCheckRotationThreshold = true;
  m_IsRotatingDuringScale = false;
  m_InAction = true;
}

void Navigator::Scale(m2::PointD const & pixelScaleCenter, double factor)
{
  ApplyScale(pixelScaleCenter, factor, m_Screen);
}

bool Navigator::ScaleImpl(m2::PointD const & newPt1, m2::PointD const & newPt2, m2::PointD const & oldPt1,
                          m2::PointD const & oldPt2, bool skipMinScaleAndBordersCheck, bool doRotateScreen,
                          ScreenBase & screen)
{
  // Project each old/new viewport touch through P3dtoP before CalcTransform;
  // perspective is non-uniform, so the 2D similarity should be built from map-plane points for both fingers.
  m2::PointD const oldP1 = screen.P3dtoP(oldPt1);
  m2::PointD const oldP2 = screen.P3dtoP(oldPt2);
  m2::PointD const newP1 = screen.P3dtoP(newPt1);
  m2::PointD const newP2 = screen.P3dtoP(newPt2);
  m2::PointD const centerG = screen.PtoG(oldP1);

  ScreenBase tmp = screen;
  tmp.SetGtoPMatrix(screen.GtoPMatrix() * ScreenBase::CalcTransform(oldP1, oldP2, newP1, newP2, doRotateScreen, true));

  if (!CheckMaxScale(tmp))
  {
    if (doRotateScreen)
      tmp.SetGtoPMatrix(screen.GtoPMatrix() *
                        ScreenBase::CalcTransform(oldP1, oldP2, newP1, newP2, doRotateScreen, false));
    else
      return false;
  }

  // SetGtoPMatrix re-derives m_Scale and (via UpdateDependentParameters) may flip
  // m_PixelRect / m_3dAngleX under auto-perspective, which leaves finger 1's geo
  // point slightly off newPt1. MatchGandP3d re-pins it. Using newPt1 (not oldPt1)
  // keeps the geo point under the moving finger.
  if (tmp.isPerspective())
    tmp.MatchGandP3d(centerG, newPt1);

  m2::RectD const & worldR = df::GetWorldRect();

  if (!skipMinScaleAndBordersCheck)
  {
    if (!CheckMinScale(tmp))
      return false;

    if (!CheckBorders(tmp))
    {
      if (!CanShrinkInto(tmp, worldR))
        return false;
      ShrinkInto(tmp, worldR);
    }
  }

  // ShrinkInto may slightly overshoot Y bounds due to floating-point rounding.
  if (!CheckBorders(tmp))
    ScaleInto(tmp, worldR);

  screen = tmp;
  return true;
}

void Navigator::DoScale(m2::PointD const & pt1, m2::PointD const & pt2)
{
  double const threshold = df::VisualParams::Instance().GetScaleThreshold();
  double const deltaPt1 = (pt1 - m_LastPt1).Length();
  double const deltaPt2 = (pt2 - m_LastPt2).Length();

  if (deltaPt1 < threshold && deltaPt2 < threshold)
    return;
  if (pt1 == pt2)
    return;

  ScreenBase PrevScreen = m_Screen;
  m_Screen = m_StartScreen;

  // Checking for rotation threshold.
  if (m_DoCheckRotationThreshold)
  {
    double s = pt1.Length(pt2) / m_StartPt1.Length(m_StartPt2);
    double a = ang::AngleTo(pt1, pt2) - ang::AngleTo(m_StartPt1, m_StartPt2);

    double aThresh = 10.0 / 180.0 * math::pi;
    double sThresh = 1.2;

    bool isScalingInBounds = (s > 1 / sThresh) && (s < sThresh);
    bool isRotationOutBounds = fabs(a) > aThresh;

    if (isScalingInBounds)
    {
      if (isRotationOutBounds)
      {
        m_IsRotatingDuringScale = true;
        m_DoCheckRotationThreshold = false;
      }
    }
    else
    {
      m_IsRotatingDuringScale = false;
      m_DoCheckRotationThreshold = false;
    }
  }

  m_Screen = PrevScreen;

  if (!ScaleImpl(pt1, pt2, m_LastPt1, m_LastPt2, pt1.Length(pt2) > m_LastPt1.Length(m_LastPt2), m_IsRotatingDuringScale,
                 m_Screen))
  {
    m_Screen = PrevScreen;
  }

  m_LastPt1 = pt1;
  m_LastPt2 = pt2;
}

void Navigator::StopScale(m2::PointD const & pt1, m2::PointD const & pt2)
{
  DoScale(pt1, pt2);
  m_InAction = false;
}

bool Navigator::IsRotatingDuringScale() const
{
  return m_IsRotatingDuringScale;
}

void Navigator::SetAutoPerspective(bool enable)
{
  m_Screen.SetAutoPerspective(enable);
}

void Navigator::Enable3dMode()
{
  if (m_Screen.isPerspective())
    return;
  double const angle = m_Screen.CalculateAutoPerspectiveAngle(m_Screen.GetScale());
  if (angle > 0)
    m_Screen.ApplyPerspective(angle, angle, m_Screen.GetAngleFOV());
}

void Navigator::SetRotationIn3dMode(double rotationAngle)
{
  m_Screen.SetRotationAngle(rotationAngle);
}

void Navigator::Disable3dMode()
{
  if (m_Screen.isPerspective())
    m_Screen.ResetPerspective();
}

m2::AnyRectD ToRotated(Navigator const & navigator, m2::RectD const & rect)
{
  double const dx = rect.SizeX();
  double const dy = rect.SizeY();

  return m2::AnyRectD(rect.Center(), ang::Angle<double>(navigator.Screen().GetAngle()),
                      m2::RectD(-dx / 2, -dy / 2, dx / 2, dy / 2));
}

void CheckMinGlobalRect(m2::RectD & rect, uint32_t tileSize, double visualScale, double scale3d)
{
  m2::RectD minRect = df::GetRectForDrawScale(scales::GetUpperStyleScale(), rect.Center(), tileSize, visualScale);
  minRect.Scale(scale3d);
  if (minRect.IsRectInside(rect))
    rect = minRect;
}

void CheckMinGlobalRect(m2::RectD & rect, double scale3d)
{
  VisualParams const & p = VisualParams::Instance();
  CheckMinGlobalRect(rect, p.GetTileSize(), p.GetVisualScale(), scale3d);
}

void CheckMinMaxVisibleScale(m2::RectD & rect, int maxScale, uint32_t tileSize, double visualScale, double scale3d)
{
  CheckMinGlobalRect(rect, tileSize, visualScale, scale3d);
  int scale = df::GetDrawTileScale(rect, tileSize, visualScale);
  if (maxScale != -1 && scale > maxScale)
  {
    // limit on passed maximal scale
    m2::PointD const c = rect.Center();
    rect = df::GetRectForDrawScale(maxScale, c, tileSize, visualScale);
    rect.Scale(scale3d);
  }
}

void CheckMinMaxVisibleScale(m2::RectD & rect, int maxScale, double scale3d)
{
  VisualParams const & p = VisualParams::Instance();
  CheckMinMaxVisibleScale(rect, maxScale, p.GetTileSize(), p.GetVisualScale(), scale3d);
}
}  // namespace df
