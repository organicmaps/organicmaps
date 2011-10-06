#include "../base/SRC_FIRST.hpp"

#include "ruler.hpp"
#include "measurement_utils.hpp"

#include "../platform/settings.hpp"

#include "../yg/overlay_renderer.hpp"
#include "../yg/skin.hpp"

#include "../indexer/mercator.hpp"
#include "../base/logging.hpp"
#include "../base/string_utils.hpp"

void Ruler::initFeets()
{
  m_units.push_back(make_pair("100 ft", 100));
  m_units.push_back(make_pair("200 ft", 200));
  m_units.push_back(make_pair("0.1 mi", 528));
  m_units.push_back(make_pair("0.2 mi", 528 * 2));
  m_units.push_back(make_pair("0.5 mi", 528 * 5));
  m_units.push_back(make_pair("1 mi", 5280));
  m_units.push_back(make_pair("2 mi", 2 * 5280));
  m_units.push_back(make_pair("5 mi", 5 * 5280));
  m_units.push_back(make_pair("10 mi", 10 * 5280));
  m_units.push_back(make_pair("20 mi", 20 * 5280));
  m_units.push_back(make_pair("50 mi", 50 * 5280));
  m_units.push_back(make_pair("100 mi", 100 * 5280));
  m_units.push_back(make_pair("200 mi", 200 * 5280));
  m_units.push_back(make_pair("500 mi", 500 * 5280));
}

void Ruler::initYards()
{
  m_units.push_back(make_pair("50 yd", 50));
  m_units.push_back(make_pair("100 yd", 100));
  m_units.push_back(make_pair("200 yd", 200));
  m_units.push_back(make_pair("500 yd", 500));
  m_units.push_back(make_pair("0.5 mi", 0.5 * 1760));
  m_units.push_back(make_pair("1 mi", 1760));
  m_units.push_back(make_pair("2 mi", 2 * 1760));
  m_units.push_back(make_pair("5 mi", 5 * 1760));
  m_units.push_back(make_pair("10 mi", 10 * 1760));
  m_units.push_back(make_pair("20 mi", 20 * 1760));
  m_units.push_back(make_pair("50 mi", 50 * 1760));
  m_units.push_back(make_pair("100 mi", 100 * 1760));
  m_units.push_back(make_pair("200 mi", 200 * 1760));
  m_units.push_back(make_pair("500 mi", 500 * 1760));
}

void Ruler::initMetres()
{
  m_units.push_back(make_pair("20 m", 20));
  m_units.push_back(make_pair("50 m", 50));
  m_units.push_back(make_pair("100 m", 100));
  m_units.push_back(make_pair("200 m", 200));
  m_units.push_back(make_pair("500 m", 500));
  m_units.push_back(make_pair("1 km", 1000));
  m_units.push_back(make_pair("2 km", 2000));
  m_units.push_back(make_pair("5 km", 5000));
  m_units.push_back(make_pair("10 km", 10000));
  m_units.push_back(make_pair("20 km", 20000));
  m_units.push_back(make_pair("50 km", 50000));
  m_units.push_back(make_pair("100 km", 100000));
  m_units.push_back(make_pair("200 km", 200000));
  m_units.push_back(make_pair("500 km", 500000));
  m_units.push_back(make_pair("1000 km", 1000000));
}

namespace {
  double identity(double val)
  {
    return val;
  }
}

Ruler::Ruler(Params const & p)
  : base_t(p), m_boundRects(1)
{
  Settings::Units units;
  Settings::Get("Units", units);
  switch (units)
  {
    case Settings::Foot:
    {
      initFeets();
      m_conversionFn = &MeasurementUtils::MetersToFeet;
      break;
    }
    case Settings::Metric:
    {
      initMetres();
      m_conversionFn = &identity;
      break;
    }
    case Settings::Yard:
    {
      initYards();
      m_conversionFn = &MeasurementUtils::MetersToYards;
      break;
    }
  }
}

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

  m_metresDiff = lonDiff / MercatorBounds::degreeInMetres * lonDiffCorrection;

  if (m_units[0].second > m_conversionFn(m_metresDiff))
  {
    m_scalerText = "<" + m_units[0].first;
    m_metresDiff = m_minUnitsWidth - 1;
  }
  else if (m_units.back().second <= m_conversionFn(m_metresDiff))
  {
    m_scalerText = ">" + m_units.back().first;
    m_metresDiff = m_maxUnitsWidth + 1;
  }
  else
    for (size_t i = 0; i < m_units.size(); ++i)
    {
      if (m_units[i].second > m_conversionFn(m_metresDiff))
      {
        m_metresDiff = m_units[i].second / m_conversionFn(1);
        m_scalerText = m_units[i].first;
        break;
      }
    }

  bool higherThanMax = m_metresDiff > m_maxUnitsWidth;
  bool lessThanMin = m_metresDiff < m_minUnitsWidth;

  /// translating metres into pixels
  double scalerWidthLatDiff = (double)m_metresDiff * MercatorBounds::degreeInMetres / lonDiffCorrection;
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

vector<m2::AnyRectD> const & Ruler::boundRects() const
{
  m_boundRects[0] = m2::AnyRectD(m_boundRect);
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

void Ruler::map(yg::StylesCache * stylesCache) const
{
}

bool Ruler::find(yg::StylesCache * stylesCache) const
{
  return true;
}

void Ruler::fillUnpacked(yg::StylesCache * stylesCache, vector<m2::PointU> & v) const
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

