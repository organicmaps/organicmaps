#include "ruler.hpp"
#include "measurement_utils.hpp"

#include "../platform/settings.hpp"

#include "../yg/overlay_renderer.hpp"
#include "../yg/skin.hpp"

#include "../indexer/mercator.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"
#include "../base/macros.hpp"


namespace
{

struct UnitValue
{
  char const * m_s;
  int m_i;
};

UnitValue g_arrFeets[] = {
  { "100 ft", 100 },
  { "200 ft", 200 },
  { "0.1 mi", 528 },
  { "0.2 mi", 528 * 2 },
  { "0.5 mi", 528 * 5 },
  { "1 mi", 5280 },
  { "2 mi", 2 * 5280 },
  { "5 mi", 5 * 5280 },
  { "10 mi", 10 * 5280 },
  { "20 mi", 20 * 5280 },
  { "50 mi", 50 * 5280 },
  { "100 mi", 100 * 5280 },
  { "200 mi", 200 * 5280 },
  { "500 mi", 500 * 5280 }
};

UnitValue g_arrYards[] = {
  { "50 yd", 50 },
  { "100 yd", 100 },
  { "200 yd", 200 },
  { "500 yd", 500 },
  { "0.5 mi", 0.5 * 1760 },
  { "1 mi", 1760 },
  { "2 mi", 2 * 1760 },
  { "5 mi", 5 * 1760 },
  { "10 mi", 10 * 1760 },
  { "20 mi", 20 * 1760 },
  { "50 mi", 50 * 1760 },
  { "100 mi", 100 * 1760 },
  { "200 mi", 200 * 1760 },
  { "500 mi", 500 * 1760 }
};

UnitValue g_arrMetres[] = {
  { "20 m", 20 },
  { "50 m", 50 },
  { "100 m", 100 },
  { "200 m", 200 },
  { "500 m", 500 },
  { "1 km", 1000 },
  { "2 km", 2000 },
  { "5 km", 5000 },
  { "10 km", 10000 },
  { "20 km", 20000 },
  { "50 km", 50000 },
  { "100 km", 100000 },
  { "200 km", 200000 },
  { "500 km", 500000 },
  { "1000 km", 1000000 }
};

  double identity(double val)
  {
    return val;
  }
}

void Ruler::setup()
{
  Settings::Units units = Settings::Metric;
  Settings::Get("Units", units);

  switch (units)
  {
  default:
    ASSERT_EQUAL ( units, Settings::Metric, () );
    m_currSystem = 0;
    m_conversionFn = &identity;
    break;

  case Settings::Foot:
    m_currSystem = 1;
    m_conversionFn = &MeasurementUtils::MetersToFeet;
    break;

  case Settings::Yard:
    m_currSystem = 2;
    m_conversionFn = &MeasurementUtils::MetersToYards;
    break;
  }

  m_isInitialized = true;
  if (m_hasPendingUpdate)
    update();
}

void Ruler::CalcMetresDiff(double v)
{
  UnitValue * arrU;
  int count;

  switch (m_currSystem)
  {
  default:
    ASSERT_EQUAL ( m_currSystem, 0, () );
    arrU = g_arrMetres;
    count = ARRAY_SIZE(g_arrMetres);
    break;

  case 1:
    arrU = g_arrFeets;
    count = ARRAY_SIZE(g_arrFeets);
    break;

  case 2:
    arrU = g_arrYards;
    count = ARRAY_SIZE(g_arrYards);
    break;
  }

  if (arrU[0].m_i > v)
  {
    m_scalerText = string("<") + arrU[0].m_s;
    m_metresDiff = m_minUnitsWidth - 1;
  }
  else if (arrU[count-1].m_i <= v)
  {
    m_scalerText = string(">") + arrU[count-1].m_s;
    m_metresDiff = m_maxUnitsWidth + 1;
  }
  else
    for (int i = 0; i < count; ++i)
    {
      if (arrU[i].m_i > v)
      {
        m_metresDiff = arrU[i].m_i / m_conversionFn(1);
        m_scalerText = arrU[i].m_s;
        break;
      }
    }
}


Ruler::Ruler(Params const & p)
  : base_t(p), m_boundRects(1), m_currSystem(0), m_isInitialized(false), m_hasPendingUpdate(false)
{
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
  if (m_isInitialized)
  {
    m2::PointD glbPivot = m_screen.PtoG(pivot());

    int rulerHeight = static_cast<int>(7 * m_visualScale);
    unsigned minPxWidth = static_cast<unsigned>(m_minPxWidth * m_visualScale);

    m2::PointD pt0 = m_screen.PtoG(pivot() - m2::PointD(minPxWidth / 2, 0));
    m2::PointD pt1 = m_screen.PtoG(pivot() + m2::PointD(minPxWidth / 2, 0));

    /// correction factor, calculated from Y-coordinate.
    double const lonDiffCorrection = cos(MercatorBounds::YToLat(glbPivot.y) / 180.0 * math::pi);

    /// longitude difference between two points
    double lonDiff = fabs(MercatorBounds::XToLon(pt0.x) - MercatorBounds::XToLon(pt1.x));

    /// converting into metres
    CalcMetresDiff(m_conversionFn(lonDiff / MercatorBounds::degreeInMetres * lonDiffCorrection));

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
  else
    m_hasPendingUpdate = true;
}

vector<m2::AnyRectD> const & Ruler::boundRects() const
{
  m_boundRects[0] = m2::AnyRectD(m_boundRect);
  return m_boundRects;
}

void Ruler::draw(yg::gl::OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const
{
  if (m_isInitialized)
  {
//    s->drawPath(
//        &m_path[0], m_path.size(), 0,
//          s->skin()->mapPenInfo(yg::PenInfo(m_fontDesc.m_color, 2 * m_visualScale, 0, 0, 0)),
//        depth() - 3);

    s->drawPath(
        &m_path[0], m_path.size(), 0,
          s->skin()->mapPenInfo(yg::PenInfo(yg::Color(0, 0, 0, 0x99), 1 * m_visualScale, 0, 0, 0)),
        depth() - 2);

    if (position() & yg::EPosLeft)
      s->drawText(m_fontDesc,
                  m_path[2] + m2::PointD(-7, -3),
                  yg::EPosAboveLeft,
                  m_scalerText.c_str(),
                  depth(),
                  false);
    else
      if (position() & yg::EPosRight)
        s->drawText(m_fontDesc,
                    m_path[1] + m2::PointD(7, -3),
                    yg::EPosAboveRight,
                    m_scalerText.c_str(),
                    depth(),
                    false);
      else
        s->drawText(m_fontDesc,
                    (m_path[1] + m_path[2]) * 0.5 + m2::PointD(0, -3),
                    yg::EPosAbove,
                    m_scalerText.c_str(),
                    depth(),
                    false);
  }
}

int Ruler::visualRank() const
{
  return 0;
}

yg::OverlayElement * Ruler::clone(math::Matrix<double, 3, 3> const & m) const
{
  return new Ruler(*this);
}

