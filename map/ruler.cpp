#include "ruler.hpp"
#include "framework.hpp"
#include "measurement_utils.hpp"

#include "../platform/settings.hpp"

#include "../gui/cached_text_view.hpp"
#include "../gui/controller.hpp"

#include "../graphics/pen.hpp"
#include "../graphics/screen.hpp"
#include "../graphics/display_list.hpp"

#include "../indexer/mercator.hpp"
#include "../geometry/distance_on_sphere.hpp"
#include "../geometry/transformations.hpp"

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

  string s;

  if (arrU[0].m_i > v)
  {
    s = string("< ") + arrU[0].m_s;
    m_metresDiff = m_minMetersWidth - 1.0;
  }
  else if (arrU[count-1].m_i <= v)
  {
    s = string("> ") + arrU[count-1].m_s;
    m_metresDiff = m_maxMetersWidth + 1.0;
  }
  else
    for (int i = 0; i < count; ++i)
    {
      if (arrU[i].m_i > v)
      {
        m_metresDiff = arrU[i].m_i / m_conversionFn(1.0);
        s = arrU[i].m_s;
        break;
      }
    }

  m_scaleText->setText(s);
}

Ruler::Params::Params()
  : m_framework(0)
{}

Ruler::Ruler(Params const & p)
  : base_t(p),
    m_cacheLength(500),
    m_boundRects(1),
    m_currSystem(0),
    m_framework(p.m_framework)
{
  gui::CachedTextView::Params pp;

  pp.m_depth = depth();

  m_scaleText.reset(new gui::CachedTextView(pp));
}

void Ruler::setController(gui::Controller * controller)
{
  gui::Element::setController(controller);
  m_scaleText->setController(controller);
}

void Ruler::layout()
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
}

void Ruler::setMinPxWidth(unsigned minPxWidth)
{
  m_minPxWidth = minPxWidth;
  setIsDirtyLayout(true);
}

void Ruler::setMinMetersWidth(double v)
{
  m_minMetersWidth = v;
  setIsDirtyLayout(true);
}

void Ruler::setMaxMetersWidth(double v)
{
  m_maxMetersWidth = v;
  setIsDirtyLayout(true);
}

void Ruler::purge()
{
  m_scaleText->purge();
  m_dl.reset();
}

void Ruler::update()
{
  double k = visualScale();

  ScreenBase const & screen = m_framework->GetNavigator().Screen();

  int const rulerHeight = my::rounds(5 * k);
  int const minPxWidth = my::rounds(m_minPxWidth * k);

  // pivot() here is the down right point of the ruler.
  // Get global points of ruler and distance according to minPxWidth.

  m2::PointD pt1 = screen.PtoG(pivot());
  m2::PointD pt0 = screen.PtoG(pivot() - m2::PointD(minPxWidth, 0));

  double const distanceInMetres = ms::DistanceOnEarth(
        MercatorBounds::YToLat(pt0.y), MercatorBounds::XToLon(pt0.x),
        MercatorBounds::YToLat(pt1.y), MercatorBounds::XToLon(pt1.x));

  // convert metres to units for calculating m_metresDiff
  CalcMetresDiff(m_conversionFn(distanceInMetres));

  bool const higherThanMax = m_metresDiff > m_maxMetersWidth;
  bool const lessThanMin = m_metresDiff < m_minMetersWidth;

  // Calculate width of the ruler in pixels.
  double scalerWidthInPx = minPxWidth;

  if (higherThanMax)
    scalerWidthInPx = minPxWidth * 3 / 2;
  else if (!lessThanMin)
  {
    // Here we need to convert metres to pixels according to angle
    // (in global coordinates) of the ruler.

    double const a = ang::AngleTo(pt1, pt0);
    pt0 = MercatorBounds::GetSmPoint(pt1, cos(a) * m_metresDiff, sin(a) * m_metresDiff);

    scalerWidthInPx = my::rounds(pivot().Length(screen.GtoP(pt0)));
  }

  m_scalerOrg = pivot() + m2::PointD(-scalerWidthInPx / 2, rulerHeight / 2);

  if (position() & graphics::EPosLeft)
    m_scalerOrg.x -= scalerWidthInPx / 2;

  if (position() & graphics::EPosRight)
    m_scalerOrg.x += scalerWidthInPx / 2;

  if (position() & graphics::EPosAbove)
    m_scalerOrg.y -= rulerHeight / 2;

  if (position() & graphics::EPosUnder)
    m_scalerOrg.y += rulerHeight / 2;

  m_scaleKoeff = scalerWidthInPx / m_cacheLength;

  if (position() & graphics::EPosLeft)
  {
    m_scaleText->setPosition(graphics::EPosAboveLeft);
    m_scaleText->setPivot(m_scalerOrg + m2::PointD(scalerWidthInPx, 0) + m2::PointD(1 * k, -2 * k));
  }
  else
    if (position() & graphics::EPosRight)
    {
      m_scaleText->setPosition(graphics::EPosAboveRight);
      m_scaleText->setPivot(m_scalerOrg + m2::PointD(7 * k, -4 * k));
    }
    else
    {
      m_scaleText->setPosition(graphics::EPosAbove);
      m_scaleText->setPivot(m_scalerOrg + m2::PointD(scalerWidthInPx / 2.0, 0) + m2::PointD(7 * k, -4 * k));
    }
}

void Ruler::setFont(gui::Element::EState state, graphics::FontDesc const & f)
{
  gui::Element::setFont(state, f);
  m_scaleText->setFont(state, f);
}

vector<m2::AnyRectD> const & Ruler::boundRects() const
{
  if (isDirtyRect())
  {
    m_boundRects[0] = m2::AnyRectD(m_scaleText->roughBoundRect());
    setIsDirtyRect(false);
  }

  return m_boundRects;
}

void Ruler::cache()
{
  layout();

  graphics::Screen * cs = m_controller->GetCacheScreen();

  m_dl.reset();
  m_dl.reset(cs->createDisplayList());

  cs->beginFrame();

  cs->setDisplayList(m_dl.get());

  cs->applySharpStates();

  double k = visualScale();

  m2::PointD path[2] = {
    m2::PointD(0, 0),
    m2::PointD(0, 0) + m2::PointD(m_cacheLength, 0)
  };

  cs->drawPath(
      path, ARRAY_SIZE(path), 0,
        cs->mapInfo(graphics::Pen::Info(graphics::Color(0, 0, 0, 0x99), 4 * k, 0, 0, 0, 0, 0, graphics::Pen::Info::ERoundJoin, graphics::Pen::Info::EButtCap)),
      depth());

  cs->setDisplayList(0);

  cs->applyStates();

  cs->endFrame();
}

void Ruler::draw(graphics::OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const
{
  if (isVisible())
  {
    checkDirtyLayout();

    s->drawDisplayList(m_dl.get(),
                       math::Shift(
                             math::Scale(m, m2::PointD(m_scaleKoeff, 1.0)),
                             m_scalerOrg));

    m_scaleText->draw(s, m);
  }
}
