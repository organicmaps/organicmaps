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
  static const int32_t MinPixelWidth = 60;
  static const int32_t MinMetersWidth = 10;
  static const int32_t MaxMetersWidth = 1000000;
  static const int32_t CacheLength = 500;

  static const int32_t MinUnitValue = -1;
  static const int32_t MaxUnitValue = numeric_limits<int32_t>::max() - 1;
  static const int32_t InvalidUnitValue = MaxUnitValue + 1;

  struct UnitValue
  {
    char const * m_s;
    int m_i;
  };

  UnitValue g_arrFeets[] = {
    { "10 ft", 10 },
    { "20 ft", 20 },
    { "50 ft", 50 },
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
    { "1 m", 1 },
    { "2 m", 2 },
    { "5 m", 5 },
    { "10 m", 10 },
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

// ========================================================= //
Ruler::RulerFrame::RulerFrame(Framework & f, const Ruler::RulerFrame::frame_end_fn & fn, double depth)
  : m_f(f)
  , m_scale(0.0)
  , m_depth(depth)
  , m_callback(fn)
{
}


Ruler::RulerFrame::RulerFrame(const Ruler::RulerFrame & other, const Ruler::RulerFrame::frame_end_fn & fn)
  : m_f(other.m_f)
{
  m_dl = other.m_dl;
  m_textDL = other.m_textDL;
  m_scale = other.m_scale;
  m_depth = other.m_depth;
  m_orgPt = other.m_orgPt;
  m_callback = fn;
  HideAnimate(false);
}

Ruler::RulerFrame::~RulerFrame()
{
  if (m_frameAnim)
  {
    m_frameAnim->Cancel();
    m_frameAnim.reset();
  }

  Purge();
}

bool Ruler::RulerFrame::IsValid() const
{
  return m_dl != NULL && m_textDL != NULL;
}

void Ruler::RulerFrame::Cache(const string & text, graphics::FontDesc const & f)
{
  gui::Controller * controller = m_f.GetGuiController();
  graphics::Screen * cs = controller->GetCacheScreen();
  double k = m_f.GetVisualScale();
  {
    m_dl.reset();
    m_dl.reset(cs->createDisplayList());

    cs->beginFrame();

    cs->setDisplayList(m_dl.get());

    cs->applyVarAlfaStates();

    double halfLength = CacheLength / 2.0;

    graphics::GlyphKey key(strings::LastUniChar("0"), f.m_size, f.m_isMasked, f.m_color);
    graphics::Glyph::Info glyphInfo(key, controller->GetGlyphCache());
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
    /*14*/  m2::PointD(CacheLength, -3.0 * k),
    /*15*/  m2::PointD(CacheLength,  0.0 * k),
    /*16*/  m2::PointD(CacheLength, -3.0 * k),
    /*17*/  m2::PointD(CacheLength, -5.0 * k),
    /*18*/  m2::PointD(CacheLength, -5.0 * k)
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
                               m_depth, glyphRes->m_pipelineID);

    cs->addTexturedStripStrided(coords + 4, sizeof(m2::PointD),
                               normals + 4, sizeof(m2::PointF),
                               &texCoords[4], sizeof(m2::PointF), ARRAY_SIZE(coords) - 4,
                               m_depth, brushRes->m_pipelineID);

    cs->setDisplayList(0);

    cs->applyStates();

    cs->endFrame();
  }

  // ============================================================ //

  ASSERT(!text.empty(), ());

  {
    m_textDL.reset();
    m_textDL.reset(cs->createDisplayList());

    cs->beginFrame();
    cs->setDisplayList(m_textDL.get());
    cs->applyVarAlfaStates();

    strings::UniString uniString = strings::MakeUniString(text);
    size_t length = uniString.size();
    buffer_vector<graphics::Glyph::Info, 8> infos(length, graphics::Glyph::Info());
    buffer_vector<graphics::Resource::Info const *, 8> resInfos(length, NULL);
    buffer_vector<uint32_t, 8> ids(length, 0);
    buffer_vector<graphics::Resource const *, 8> glyphRes(length, NULL);

    for (size_t i = 0; i < uniString.size(); ++i)
    {
      infos[i] = graphics::Glyph::Info(graphics::GlyphKey(uniString[i], f.m_size, false, f.m_color),
                                       controller->GetGlyphCache());

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
                                 coords.size(), m_depth, pipelineID);
    }

    cs->setDisplayList(0);
    cs->endFrame();
  }
}

void Ruler::RulerFrame::Purge()
{
  m_dl.reset();
  m_textDL.reset();
}

bool Ruler::RulerFrame::IsHidingAnim() const
{
  ASSERT(m_frameAnim != NULL, ());
  AlfaAnimationTask * a = static_cast<AlfaAnimationTask *>(m_frameAnim.get());
  return a->IsHiding();
}

bool Ruler::RulerFrame::IsAnimActive() const
{
  return m_frameAnim != NULL;
}

void Ruler::RulerFrame::SetScale(double scale)
{
  m_scale = scale;
}

double Ruler::RulerFrame::GetScale() const
{
  return m_scale;
}

void Ruler::RulerFrame::SetOrgPoint(const m2::PointD & org)
{
  m_orgPt = org;
}

const m2::PointD &Ruler::RulerFrame::GetOrgPoint() const
{
  return m_orgPt;
}

void Ruler::RulerFrame::ShowAnimate(bool needPause)
{
  double offset = (needPause == true) ? 0.07 : 0.0;
  CreateAnim(IsAnimActive() ? GetCurrentAlfa() : 0.1, 1.0, 0.2, offset, true);
}

void Ruler::RulerFrame::HideAnimate(bool needPause)
{
  double offset = (needPause == true) ? 1.0 : 0.0;
  double timeInterval = (needPause == true) ? 0.3 : 0.15;
  CreateAnim(1.0, 0.0, timeInterval, offset, false);
}

void Ruler::RulerFrame::Draw(graphics::OverlayRenderer * r, const math::Matrix<double, 3, 3> & m)
{
  LOG(LINFO, ("Main dl = ", m_dl.get()));
  ASSERT(m_dl != NULL, ());
  ASSERT(m_textDL != NULL, ());
  graphics::UniformsHolder holder;
  float a = GetCurrentAlfa();
  holder.insertValue(graphics::ETransparency, a);

  r->drawDisplayList(m_dl.get(), math::Shift(
                           math::Scale(m, m2::PointD(m_scale, 1.0)),
                            m_orgPt), &holder);

  double yOffset = -(2 + 5 * m_f.GetVisualScale());
  r->drawDisplayList(m_textDL.get(),
                     math::Shift(m, m_orgPt + m2::PointF(CacheLength * m_scale, yOffset)),
                     &holder);
}

void Ruler::RulerFrame::CreateAnim(double startAlfa, double endAlfa, double timeInterval, double timeOffset, bool isVisibleAtEnd)
{
  anim::Controller * animController = m_f.GetAnimController();
  if (animController == NULL)
    return;

  if (m_frameAnim)
    m_frameAnim->Cancel();

  m_frameAnim.reset(new AlfaAnimationTask(startAlfa, endAlfa, timeInterval, timeOffset, &m_f));
  m_frameAnim->AddCallback(anim::Task::EEnded, bind(&RulerFrame::AnimEnded, this, isVisibleAtEnd));
  animController->AddTask(m_frameAnim);
}

float Ruler::RulerFrame::GetCurrentAlfa()
{
  if (m_frameAnim)
  {
    AlfaAnimationTask * a = static_cast<AlfaAnimationTask *>(m_frameAnim.get());
    return a->GetCurrentAlfa();
  }

  return 1.0;
}

void Ruler::RulerFrame::AnimEnded(bool isVisible)
{
  if (m_frameAnim != NULL)
  {
    m_frameAnim.reset();
    m_callback(isVisible, this);
  }
}
// ========================================================= //

double Ruler::CalcMetresDiff(double value)
{
  UnitValue * arrU = g_arrMetres;
  int count = ARRAY_SIZE(g_arrMetres);

  typedef double (*ConversionFn)(double);
  ConversionFn conversionFn = &identity;

  switch (m_currSystem)
  {
  default:
    ASSERT_EQUAL ( m_currSystem, 0, () );
    break;

  case 1:
    arrU = g_arrFeets;
    count = ARRAY_SIZE(g_arrFeets);
    conversionFn = &MeasurementUtils::MetersToFeet;
    break;

  case 2:
    arrU = g_arrYards;
    count = ARRAY_SIZE(g_arrYards);
    conversionFn = &MeasurementUtils::MetersToYards;
    break;
  }

  int prevUnitRange = m_currentRangeIndex;
  string s;
  double result = 0.0;
  double v = conversionFn(value);
  if (arrU[0].m_i > v)
  {
    m_currentRangeIndex = MinUnitValue;
    s = string("< ") + arrU[0].m_s;
    result = MinMetersWidth - 1.0;
  }
  else if (arrU[count-1].m_i <= v)
  {
    m_currentRangeIndex = MaxUnitValue;
    s = string("> ") + arrU[count-1].m_s;
    result = MaxMetersWidth + 1.0;
  }
  else
    for (int i = 0; i < count; ++i)
    {
      if (arrU[i].m_i > v)
      {
        m_currentRangeIndex = i;
        result = arrU[i].m_i / conversionFn(1.0);
        s = arrU[i].m_s;
        break;
      }
    }

  if (m_currentRangeIndex != prevUnitRange)
    UpdateText(s);
  return result;
}

Ruler::Params::Params()
  : m_framework(0)
{}

Ruler::Ruler(Params const & p)
  : base_t(p),
    m_boundRects(1),
    m_currentRangeIndex(InvalidUnitValue),
    m_currSystem(0),
    m_mainFrame(NULL),
    m_animFrames(NULL),
    m_framework(p.m_framework)
{
  setIsVisible(false);
}

void Ruler::AnimateShow()
{
  RulerFrame * frame = GetMainFrame();
  if (!isVisible() && (!frame->IsAnimActive() || frame->IsHidingAnim()))
  {
    setIsVisible(true);
    frame->ShowAnimate(false);
  }
  else if (isVisible() && (frame->IsAnimActive() && frame->IsHidingAnim()))
    frame->ShowAnimate(false);
}

void Ruler::AnimateHide()
{
  RulerFrame * frame = GetMainFrame();
  if (isVisible() && (!frame->IsAnimActive() || !frame->IsHidingAnim()))
    frame->HideAnimate(true);
}

void Ruler::layout()
{
  Settings::Units units = Settings::Metric;
  Settings::Get("Units", units);

  int prevCurrSystem = m_currSystem;
  switch (units)
  {
  default:
    ASSERT_EQUAL ( units, Settings::Metric, () );
    m_currSystem = 0;
    break;
  case Settings::Foot:
    m_currSystem = 1;
    break;
  case Settings::Yard:
    m_currSystem = 2;
    break;
  }

  if (prevCurrSystem != m_currSystem)
    m_currentRangeIndex = InvalidUnitValue;

  update();
}

void Ruler::UpdateText(const string & text)
{
  RulerFrame * frame = GetMainFrame();
  if (frame->IsValid())
  {
    //RulerFrame * addFrame = new RulerFrame(*frame, bind(&Ruler::AnimFrameAnimEnded, this, _1, _2));
    //m_animFrames.push_back(addFrame);
    delete m_animFrames;
    m_animFrames = new RulerFrame(*frame, bind(&Ruler::AnimFrameAnimEnded, this, _1, _2));
  }

  frame->Cache(text, font(EActive));
  if (isVisible())
    frame->ShowAnimate(true);
}

void Ruler::MainFrameAnimEnded(bool isVisible, RulerFrame * frame)
{
  setIsVisible(isVisible);
  ASSERT(GetMainFrame() == frame, ());
}

void Ruler::AnimFrameAnimEnded(bool /*isVisible*/, RulerFrame * frame)
{
  //m_animFrames.remove(frame);
  //delete frame;
  delete frame;
  m_animFrames = NULL;
}

Ruler::RulerFrame * Ruler::GetMainFrame()
{
  if (m_mainFrame == NULL)
    m_mainFrame = new RulerFrame(*m_framework, bind(&Ruler::MainFrameAnimEnded, this, _1, _2), depth());

  return m_mainFrame;
}

Ruler::RulerFrame * Ruler::GetMainFrame() const
{
  ASSERT(m_mainFrame != NULL, ());
  return m_mainFrame;
}

void Ruler::purge()
{
  m_currentRangeIndex = InvalidUnitValue;
  delete m_mainFrame;
  m_mainFrame = NULL;

  //GetRangeDeletor(m_animFrames, DeleteFunctor())();
  delete m_animFrames;
  m_animFrames = NULL;
  setIsVisible(false);
}

void Ruler::update()
{
  double k = visualScale();

  ScreenBase const & screen = m_framework->GetNavigator().Screen();

  int const rulerHeight = my::rounds(5 * k);
  int const minPxWidth = my::rounds(MinPixelWidth * k);

  // pivot() here is the down right point of the ruler.
  // Get global points of ruler and distance according to minPxWidth.

  m2::PointD pt1 = screen.PtoG(pivot());
  m2::PointD pt0 = screen.PtoG(pivot() - m2::PointD(minPxWidth, 0));

  double const distanceInMetres = ms::DistanceOnEarth(
        MercatorBounds::YToLat(pt0.y), MercatorBounds::XToLon(pt0.x),
        MercatorBounds::YToLat(pt1.y), MercatorBounds::XToLon(pt1.x));

  // convert metres to units for calculating m_metresDiff
  double metersDiff = CalcMetresDiff(distanceInMetres);

  bool const higherThanMax = metersDiff > MaxMetersWidth;
  bool const lessThanMin = metersDiff < MinMetersWidth;

  // Calculate width of the ruler in pixels.
  double scalerWidthInPx = minPxWidth;

  if (higherThanMax)
    scalerWidthInPx = minPxWidth * 3 / 2;
  else if (!lessThanMin)
  {
    // Here we need to convert metres to pixels according to angle
    // (in global coordinates) of the ruler.

    double const a = ang::AngleTo(pt1, pt0);
    pt0 = MercatorBounds::GetSmPoint(pt1, cos(a) * metersDiff, sin(a) * metersDiff);

    scalerWidthInPx = my::rounds(pivot().Length(screen.GtoP(pt0)));
  }

  m2::PointD orgPt = pivot() + m2::PointD(-scalerWidthInPx / 2, rulerHeight / 2);

  if (position() & graphics::EPosLeft)
    orgPt.x -= scalerWidthInPx / 2;

  if (position() & graphics::EPosRight)
    orgPt.x += scalerWidthInPx / 2;

  if (position() & graphics::EPosAbove)
    orgPt.y -= rulerHeight / 2;

  if (position() & graphics::EPosUnder)
    orgPt.y += rulerHeight / 2;

  RulerFrame * frame = GetMainFrame();
  frame->SetScale(scalerWidthInPx / CacheLength);
  frame->SetOrgPoint(orgPt);
}

vector<m2::AnyRectD> const & Ruler::boundRects() const
{
  if (isDirtyRect())
  {
    graphics::FontDesc const & f = font(EActive);
    RulerFrame * frame = GetMainFrame();
    m2::RectD rect(frame->GetOrgPoint(), m2::PointD(CacheLength * frame->GetScale(), f.m_size * 2));
    m_boundRects[0] = m2::AnyRectD(rect);
    setIsDirtyRect(false);
  }

  return m_boundRects;
}

void Ruler::cache()
{
  (void)GetMainFrame();
  update();
}

void Ruler::draw(graphics::OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const
{
  if (isVisible())
  {
    checkDirtyLayout();

    RulerFrame * frame = GetMainFrame();
    frame->Draw(s, m);
    //for_each(m_animFrames.begin(), m_animFrames.end(), bind(&Ruler::RulerFrame::Draw, _1, s, m));
    if (m_animFrames)
      m_animFrames->Draw(s, m);
  }
}
