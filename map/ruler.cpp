#include "../base/SRC_FIRST.hpp"

#include "ruler.hpp"

#include "../yg/overlay_renderer.hpp"
#include "../yg/skin.hpp"

#include "../indexer/mercator.hpp"
#include "../base/logging.hpp"
#include "../base/string_utils.hpp"

Ruler::Ruler(Params const & p)
  : base_t(p), m_boundRects(1)
{}

void Ruler::setScreen(ScreenBase const & screen)
{
  m_screen = screen;
}

ScreenBase const & Ruler::screen() const
{
  return m_screen;
}

void Ruler::setMinPxWidth(unsigned minPxWidth)
{
  m_minPxWidth = minPxWidth;
}

void Ruler::setMinUnitsWidth(double minUnitsWidth)
{
  m_minUnitsWidth = minUnitsWidth;
}

void Ruler::setMaxUnitsWidth(double maxUnitsWidth)
{
  m_maxUnitsWidth = maxUnitsWidth;
}

void Ruler::setVisualScale(double visualScale)
{
  m_visualScale = visualScale;
}

void Ruler::setFontDesc(yg::FontDesc const & fontDesc)
{
  m_fontDesc = fontDesc;
}

unsigned Ruler::ceil(double unitsDiff)
{
  /// finding the closest higher metric value
  double curVal = m_minUnitsWidth;

  unsigned curFirstDigit = (unsigned)m_minUnitsWidth;
  while (curFirstDigit > 10)
    curFirstDigit /= 10;

  if (unitsDiff > m_maxUnitsWidth)
    curVal = m_maxUnitsWidth + 1;
  else
  if (unitsDiff < m_minUnitsWidth)
    curVal = m_minUnitsWidth - 1;
  else
    while (true)
    {
      double nextVal = curFirstDigit == 2 ? (curVal * 5 / 2) : curVal * 2;
      unsigned nextFirstDigit = curFirstDigit == 2 ? (curFirstDigit * 5 / 2) : curFirstDigit * 2;

      if (nextFirstDigit >= 10)
        nextFirstDigit /= 10;

      if ((curVal <= unitsDiff) && (nextVal > unitsDiff))
      {
        curVal = nextVal;
        curFirstDigit = nextFirstDigit;
        break;
      }

      curVal = nextVal;
      curFirstDigit = nextFirstDigit;
    }

  return (unsigned)curVal;
}

void Ruler::update()
{
  m2::PointD glbPivot = m_screen.PtoG(pivot());

  int rulerHeight = static_cast<int>(14 * m_visualScale);
  unsigned minPxWidth = static_cast<unsigned>(m_minPxWidth * m_visualScale);

  m2::PointD pt0 = m_screen.PtoG(pivot() - m2::PointD(minPxWidth / 2, 0));
  m2::PointD pt1 = m_screen.PtoG(pivot() + m2::PointD(minPxWidth / 2, 0));

  /// correction factor, calculated from Y-coordinate.
  double const lonDiffCorrection = cos(MercatorBounds::YToLat(glbPivot.y) / 180.0 * math::pi);

  /// longitude difference between two points
  double lonDiff = fabs(MercatorBounds::XToLon(pt0.x) - MercatorBounds::XToLon(pt1.x));

  /// converting into metres
  /// TODO : calculate in different units

  m_unitsDiff = lonDiff / MercatorBounds::degreeInMetres * lonDiffCorrection;
  m_unitsDiff = ceil(m_unitsDiff);

  /// updating scaler text

  bool higherThanMax = m_unitsDiff > m_maxUnitsWidth;
  bool lessThanMin = m_unitsDiff < m_minUnitsWidth;

  double textUnitsDiff = m_unitsDiff;

  m_scalerText = "";
  if (higherThanMax)
  {
    m_scalerText = ">";
    textUnitsDiff = m_maxUnitsWidth;
  }
  else
    if (lessThanMin)
    {
      m_scalerText = "<";
      textUnitsDiff = m_minUnitsWidth;
    }

  if (m_unitsDiff >= 1000)
    m_scalerText += strings::to_string(textUnitsDiff / 1000) + " km";
  else
    m_scalerText += strings::to_string(textUnitsDiff) + " m";

  /// translating units into pixels
  double scalerWidthLatDiff = (double)m_unitsDiff * MercatorBounds::degreeInMetres / lonDiffCorrection;
  double scalerWidthXDiff = MercatorBounds::LonToX(glbPivot.x + scalerWidthLatDiff / 2)
                          - MercatorBounds::LonToX(glbPivot.x - scalerWidthLatDiff / 2);

  double scalerWidthInPx = m_screen.GtoP(glbPivot).x - m_screen.GtoP(glbPivot + m2::PointD(scalerWidthXDiff, 0)).x;
  scalerWidthInPx = (higherThanMax || lessThanMin) ? minPxWidth : abs(my::rounds(scalerWidthInPx));

  m2::PointD scalerOrg = pivot() + m2::PointD(-scalerWidthInPx / 2, rulerHeight / 2);

  if (position() & yg::EPosLeft)
    scalerOrg.x -= scalerWidthInPx / 2;

  if (position() & yg::EPosRight)
    scalerOrg.x += scalerWidthInPx / 2;

  if (position() & yg::EPosAbove)
    scalerOrg.y -= rulerHeight / 2;

  if (position() & yg::EPosUnder)
    scalerOrg.y += rulerHeight / 2;

  m_path.clear();
  m_path.push_back(scalerOrg + m2::PointD(0, -rulerHeight));
  m_path.push_back(scalerOrg);
  m_path.push_back(scalerOrg + m2::PointD(scalerWidthInPx, 0));
  m_path.push_back(m_path[2] + m2::PointD(0, -rulerHeight));

  /// calculating bound rect

  m_boundRect = m2::RectD(m_path[0], m_path[0]);
  m_boundRect.Add(m_path[1]);
  m_boundRect.Add(m_path[2]);
  m_boundRect.Add(m_path[3]);
}

vector<m2::AARectD> const & Ruler::boundRects() const
{
  m_boundRects[0] = m2::AARectD(m_boundRect);
  return m_boundRects;
}

void Ruler::draw(yg::gl::OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const
{
  s->drawPath(
      &m_path[0], m_path.size(), 0,
        s->skin()->mapPenInfo(yg::PenInfo(yg::Color(255, 255, 255, 255), 2 * m_visualScale, 0, 0, 0)),
      depth() - 3);

  s->drawPath(
      &m_path[0], m_path.size(), 0,
      s->skin()->mapPenInfo(yg::PenInfo(yg::Color(0, 0, 0, 255), 1 * m_visualScale, 0, 0, 0)),
      depth() - 2);

  if (position() & yg::EPosLeft)
    s->drawText(m_fontDesc,
                m_path[2] + m2::PointD(-7, -7),
                yg::EPosAboveLeft,
                m_scalerText.c_str(),
                depth(),
                false);
  else
    if (position() & yg::EPosRight)
      s->drawText(m_fontDesc,
                  m_path[1] + m2::PointD(7, -7),
                  yg::EPosAboveRight,
                  m_scalerText.c_str(),
                  depth(),
                  false);
    else
      s->drawText(m_fontDesc,
                  (m_path[1] + m_path[2]) * 0.5 + m2::PointD(0, -7),
                  yg::EPosAbove,
                  m_scalerText.c_str(),
                  depth(),
                  false);

}

void Ruler::cache(yg::StylesCache * stylesCache) const
{
}

int Ruler::visualRank() const
{
  return 0;
}

yg::OverlayElement * Ruler::clone(math::Matrix<double, 3, 3> const & m) const
{
  return new Ruler(*this);
}

