#include "ruler.hpp"
#include "framework.hpp"
#include "measurement_utils.hpp"
#include "alfa_animation_task.hpp"

#include "../anim/controller.hpp"

#include "../platform/settings.hpp"

#include "../gui/cached_text_view.hpp"
#include "../gui/controller.hpp"

#include "../graphics/glyph.hpp"
#include "../graphics/brush.hpp"
#include "../graphics/screen.hpp"
#include "../graphics/display_list.hpp"
#include "../graphics/uniforms_holder.hpp"

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
    { "0.5 mi", 1760 / 2 },
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

void Ruler::AlfaAnimEnded(bool isVisible)
{
  setIsVisible(isVisible);
  m_rulerAnim.reset();
}

bool Ruler::IsHidingAnim() const
{
  ASSERT(m_rulerAnim != NULL, ());
  AlfaCompassAnim * a = static_cast<AlfaCompassAnim *>(m_rulerAnim.get());
  return a->IsHiding();
}

float Ruler::GetCurrentAlfa() const
{
  if (m_rulerAnim)
  {
    AlfaCompassAnim * a = static_cast<AlfaCompassAnim *>(m_rulerAnim.get());
    return a->GetCurrentAlfa();
  }

  return isVisible() ? 1.0 : 0.0;
}

void Ruler::CreateAnim(double startAlfa, double endAlfa, double timeInterval, double timeOffset, bool isVisibleAtEnd)
{
  if (m_framework->GetAnimController() == NULL)
    return;

  if (m_rulerAnim)
    m_rulerAnim->Cancel();
  m_rulerAnim.reset(new AlfaCompassAnim(startAlfa, endAlfa, timeInterval, timeOffset, m_framework));
  m_rulerAnim->AddCallback(anim::Task::EEnded, bind(&Ruler::AlfaAnimEnded, this, isVisibleAtEnd));
  m_framework->GetAnimController()->AddTask(m_rulerAnim);
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

  UpdateText(s);
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

  m_dl = NULL;
  memset(m_textDL, 0, ARRAY_SIZE(m_textDL) * sizeof(void *));
}

void Ruler::AnimateShow()
{
  if (!isVisible() && (m_rulerAnim == NULL || IsHidingAnim()))
  {
    setIsVisible(true);
    CreateAnim(0.1, 1.0, 0.2, 0.0, true);
  }

  if (isVisible() && (m_rulerAnim == NULL || IsHidingAnim()))
    CreateAnim(GetCurrentAlfa(), 1.0, 0.2, 0.0, true);
}

void Ruler::AnimateHide()
{
  if (isVisible() && (m_rulerAnim == NULL || !IsHidingAnim()))
    CreateAnim(1.0, 0.0, 0.3, 1.0, false);
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

  update();
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

void Ruler::PurgeMainDL()
{
  delete m_dl;
  m_dl = NULL;
}

void Ruler::CacheMainDL()
{
  graphics::Screen * cs = m_controller->GetCacheScreen();

  PurgeMainDL();
  m_dl = cs->createDisplayList();

  cs->beginFrame();

  cs->setDisplayList(m_dl);

  cs->applyVarAlfaStates();

  double k = visualScale();
  double halfLength = m_cacheLength / 2.0;

  graphics::FontDesc const & f = font(EActive);
  graphics::GlyphKey key(strings::LastUniChar("0"), f.m_size, f.m_isMasked, f.m_color);
  graphics::Glyph::Info glyphInfo(key, m_controller->GetGlyphCache());
  uint32_t zeroMarkGlyph = cs->mapInfo(glyphInfo);
  graphics::Resource const * glyphRes = cs->fromID(zeroMarkGlyph);

  m2::RectI glyphRect(glyphRes->m_texRect);
  double glyphHalfW = glyphRect.SizeX() / 2.0;
  double glyphHalfH = glyphRect.SizeY() / 2.0;
  double zeroMarkOffset = (glyphHalfH + 2) + 5 * k;

  graphics::Brush::Info brushInfo(graphics::Color(3, 3, 3, 255));
  uint32_t brushId = cs->mapInfo(brushInfo);
  graphics::Resource const * brushRes = cs->fromID(brushId);
  m2::RectU brushRect = brushRes->m_texRect;

  shared_ptr<graphics::gl::BaseTexture> glyphTexture = cs->pipeline(glyphRes->m_pipelineID).texture();
  m2::PointF brushCenter = cs->pipeline(brushRes->m_pipelineID).texture()->mapPixel(brushRect.Center());

  //  0  1     10  11        18  17
  //   --        --            --
  //   ||        ||            ||
  //   || 3, 6  9||8, 12     16||14
  //  2|---------  -------------|
  //   |                        |
  //   |                        |
  //  4--------------------------
  //    5         7, 13         15

  m2::PointD coords[] =
  {
  //      Zero mark geometry
  /*-4*/  m2::PointD(0.0, -zeroMarkOffset),
  /*-3*/  m2::PointD(0.0, -zeroMarkOffset),
  /*-2*/  m2::PointD(0.0, -zeroMarkOffset),
  /*-1*/  m2::PointD(0.0, -zeroMarkOffset),
  //      Ruler geometry
  /* 0*/  m2::PointD(0.0, -5.0 * k),
  /* 1*/  m2::PointD(0.0, -5.0 * k),
  /* 2*/  m2::PointD(0.0, -3.0 * k),
  /* 3*/  m2::PointD(0.0, -3.0 * k),
  /* 4*/  m2::PointD(0.0,  0.0),
  /* 5*/  m2::PointD(0.0,  0.0),
  /* 6*/  m2::PointD(0.0, -3.0 * k),
  /* 7*/  m2::PointD(halfLength - 0.5,  0.0),
  /* 8*/  m2::PointD(halfLength - 0.5, -3.0 * k),
  /* 9*/  m2::PointD(halfLength - 0.5, -3.0 * k),
  /*10*/  m2::PointD(halfLength - 0.5, -7.0 * k),
  /*11*/  m2::PointD(halfLength - 0.5, -7.0 * k),
  /*12*/  m2::PointD(halfLength - 0.5, -3.0 * k),
  /*13*/  m2::PointD(halfLength - 0.5,  0.0 * k),
  /*14*/  m2::PointD(m_cacheLength, -3.0 * k),
  /*15*/  m2::PointD(m_cacheLength,  0.0 * k),
  /*16*/  m2::PointD(m_cacheLength, -3.0 * k),
  /*17*/  m2::PointD(m_cacheLength, -5.0 * k),
  /*18*/  m2::PointD(m_cacheLength, -5.0 * k)
  };

  m2::PointF normals[] =
  {
  //      Zero mark normals
  /*-4*/  m2::PointF(-glyphHalfW + 1, -glyphHalfH),
  /*-3*/  m2::PointF(-glyphHalfW + 1,  glyphHalfH),
  /*-2*/  m2::PointF( glyphHalfW + 1, -glyphHalfH),
  /*-1*/  m2::PointF( glyphHalfW + 1,  glyphHalfH),
  //      Ruler normals
  /* 0*/  m2::PointF( 0.0     , 0.0),
  /* 1*/  m2::PointF( 1.0 * k , 0.0),
  /* 2*/  m2::PointF( 0.0     , 0.0),
  /* 3*/  m2::PointF( 1.0 * k , 0.0),
  /* 4*/  m2::PointF( 0.0     , 0.0),
  /* 5*/  m2::PointF( 1.0 * k , 0.0),
  /* 6*/  m2::PointF( 1.0 * k , 0.0),
  /* 7*/  m2::PointF( 1.0 * k , 0.0),
  /* 8*/  m2::PointF( 1.0 * k , 0.0),
  /* 9*/  m2::PointF( 0.0     , 0.0),
  /*10*/  m2::PointF( 0.0     , 0.0),
  /*11*/  m2::PointF( 1.0 * k , 0.0),
  /*12*/  m2::PointF( 1.0 * k , 0.0),
  /*13*/  m2::PointF( 1.0 * k , 0.0),
  /*14*/  m2::PointF( 0.0     , 0.0),
  /*15*/  m2::PointF( 0.0     , 0.0),
  /*16*/  m2::PointF(-1.0 * k , 0.0),
  /*17*/  m2::PointF( 0.0     , 0.0),
  /*18*/  m2::PointF(-1.0 * k , 0.0)
  };

  vector<m2::PointF> texCoords(ARRAY_SIZE(normals), brushCenter);
  texCoords[0] = glyphTexture->mapPixel(m2::PointF(glyphRect.minX(), glyphRect.minY()));
  texCoords[1] = glyphTexture->mapPixel(m2::PointF(glyphRect.minX(), glyphRect.maxY()));
  texCoords[2] = glyphTexture->mapPixel(m2::PointF(glyphRect.maxX(), glyphRect.minY()));
  texCoords[3] = glyphTexture->mapPixel(m2::PointF(glyphRect.maxX(), glyphRect.maxY()));

  ASSERT(ARRAY_SIZE(coords) == ARRAY_SIZE(normals), ());

  cs->addTexturedStripStrided(coords, sizeof(m2::PointD),
                             normals, sizeof(m2::PointF),
                             &texCoords[0], sizeof(m2::PointF), 4,
                             depth(), glyphRes->m_pipelineID);

  cs->addTexturedStripStrided(coords + 4, sizeof(m2::PointD),
                             normals + 4, sizeof(m2::PointF),
                             &texCoords[4], sizeof(m2::PointF), ARRAY_SIZE(coords) - 4,
                             depth(), brushRes->m_pipelineID);

  cs->setDisplayList(0);

  cs->applyStates();

  cs->endFrame();
}

void Ruler::PurgeTextDL(int index)
{
  delete m_textDL[index];
  m_textDL[index] = NULL;
}

void Ruler::UpdateText(const string & text)
{
  ASSERT(!text.empty(), ());
  ASSERT(m_textDL[1] == NULL, ());
  swap(m_textDL[0], m_textDL[1]);
  PurgeTextDL(1);

  graphics::Screen * cs = m_controller->GetCacheScreen();
  m_textDL[0] = cs->createDisplayList();

  cs->beginFrame();
  cs->setDisplayList(m_textDL[0]);
  cs->applyVarAlfaStates();

  strings::UniString uniString = strings::MakeUniString(text);
  size_t length = uniString.size();
  buffer_vector<graphics::Glyph::Info, 8> infos(length, graphics::Glyph::Info());
  buffer_vector<graphics::Resource::Info const *, 8> resInfos(length, NULL);
  buffer_vector<uint32_t, 8> ids(length, 0);
  buffer_vector<graphics::Resource const *, 8> glyphRes(length, NULL);

  graphics::FontDesc const & f = font(EActive);

  for (size_t i = 0; i < uniString.size(); ++i)
  {
    infos[i] = graphics::Glyph::Info(graphics::GlyphKey(uniString[i], f.m_size, false, f.m_color),
                                     m_controller->GetGlyphCache());

    resInfos[i] = &infos[i];
  }

  if (cs->mapInfo(resInfos.data(), ids.data(), infos.size()))
  {
    uint32_t width = 0;
    for (size_t i = 0; i < ids.size(); ++i)
    {
      graphics::Resource const * res = cs->fromID(ids[i]);
      width += res->m_texRect.SizeX();
      glyphRes[i] = res;
    }

    int32_t pipelineID = glyphRes[0]->m_pipelineID;
    shared_ptr<graphics::gl::BaseTexture> texture = cs->pipeline(pipelineID).texture();
    double halfWidth = width / 2.0;
    double lengthFromStart = 0.0;

    buffer_vector<m2::PointF, 48> coords;
    buffer_vector<m2::PointF, 48> normals;
    buffer_vector<m2::PointF, 48> texCoords;

    for (size_t i = 0; i < uniString.size(); ++i)
    {
      double baseX = lengthFromStart - halfWidth;
      coords.push_back(m2::PointD(baseX, 0.0));
      coords.push_back(m2::PointD(baseX, 0.0));
      coords.push_back(m2::PointD(baseX, 0.0));

      coords.push_back(m2::PointD(baseX, 0.0));
      coords.push_back(m2::PointD(baseX, 0.0));
      coords.push_back(m2::PointD(baseX, 0.0));

      m2::RectI resourceRect(glyphRes[i]->m_texRect);
      double w = resourceRect.SizeX();
      double h = resourceRect.SizeY();
      lengthFromStart += w;

      normals.push_back(m2::PointF(0.0, 0.0));
      normals.push_back(m2::PointF(0.0, -h));
      normals.push_back(m2::PointF(w  , 0.0));

      normals.push_back(m2::PointF(w  , 0.0));
      normals.push_back(m2::PointF(0.0, -h));
      normals.push_back(m2::PointF(w  , -h));

      texCoords.push_back(texture->mapPixel(m2::PointF(resourceRect.minX(), resourceRect.maxY())));
      texCoords.push_back(texture->mapPixel(m2::PointF(resourceRect.minX(), resourceRect.minY())));
      texCoords.push_back(texture->mapPixel(m2::PointF(resourceRect.maxX(), resourceRect.maxY())));

      texCoords.push_back(texture->mapPixel(m2::PointF(resourceRect.maxX(), resourceRect.maxY())));
      texCoords.push_back(texture->mapPixel(m2::PointF(resourceRect.minX(), resourceRect.minY())));
      texCoords.push_back(texture->mapPixel(m2::PointF(resourceRect.maxX(), resourceRect.minY())));
    }

    cs->addTexturedListStrided(coords.data(), sizeof(m2::PointF),
                               normals.data(), sizeof(m2::PointF),
                               texCoords.data(), sizeof(m2::PointF),
                               coords.size(), depth(), pipelineID);
  }

  cs->setDisplayList(0);
  cs->endFrame();
}

void Ruler::purge()
{
  PurgeMainDL();
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
}

vector<m2::AnyRectD> const & Ruler::boundRects() const
{
  if (isDirtyRect())
  {
    // TODO
    graphics::FontDesc const & f = font(EActive);
    m2::RectD rect(m_scalerOrg, m2::PointD(m_cacheLength * m_scaleKoeff, f.m_size * 2));
    m_boundRects[0] = m2::AnyRectD(rect);
    setIsDirtyRect(false);
  }

  return m_boundRects;
}

void Ruler::cache()
{
  CacheMainDL();
}

void Ruler::draw(graphics::OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const
{
  if (isVisible())
  {
    checkDirtyLayout();

    graphics::UniformsHolder holder;
    holder.insertValue(graphics::ETransparency, GetCurrentAlfa());

    s->drawDisplayList(m_dl, math::Shift(
                              math::Scale(m, m2::PointD(m_scaleKoeff, 1.0)),
                              m_scalerOrg), &holder);

    double yOffset = -(2 + 5 * m_controller->GetVisualScale());
    if (m_textDL[0])
      s->drawDisplayList(m_textDL[0],
                          math::Shift(m, m_scalerOrg + m2::PointF(m_cacheLength * m_scaleKoeff, yOffset)));
  }
}
