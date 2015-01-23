#include "map/ruler.hpp"
#include "map/framework.hpp"
#include "map/alfa_animation_task.hpp"

#include "anim/controller.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include "indexer/mercator.hpp"

#include "geometry/transformations.hpp"

#include "base/string_utils.hpp"

namespace
{
  static const int RulerHeight = 2;
  static const int MinPixelWidth = 60;
  static const int MinMetersWidth = 10;
  static const int MaxMetersWidth = 1000000;
  static const int CacheLength = 500;

  static const int MinUnitValue = -1;
  static const int MaxUnitValue = numeric_limits<int>::max() - 1;
  static const int InvalidUnitValue = MaxUnitValue + 1;

  static const int TextOffsetFromRuler = 3;

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
  , m_textLengthInPx(0)
  , m_scale(0.0)
  , m_depth(depth)
  , m_callback(fn)
{
}


Ruler::RulerFrame::RulerFrame(const Ruler::RulerFrame & other, const Ruler::RulerFrame::frame_end_fn & fn)
  : m_f(other.m_f)
{
  m_textLengthInPx = other.m_textLengthInPx;
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
  return false;
}

///@TODO UVR
//void Ruler::RulerFrame::Cache(const string & text, FontDesc const & f)
//{
//  gui::Controller * controller = m_f.GetGuiController();
//  Screen * cs = controller->GetCacheScreen();
//  double const k = m_f.GetVisualScale();

//  // Create solid line DL.
//  if (m_dl == NULL)
//  {
//    m_dl.reset(cs->createDisplayList());

//    cs->beginFrame();
//    cs->setDisplayList(m_dl.get());
//    cs->applyVarAlfaStates();

//    m2::PointD coords[] =
//    {
//    /* 3*/  m2::PointD(0.0, -RulerHeight * k),
//    /* 4*/  m2::PointD(0.0,  0.0),
//    /*14*/  m2::PointD(CacheLength, -RulerHeight * k),
//    /*15*/  m2::PointD(CacheLength,  0.0 * k),
//    };

//    Brush::Info const brushInfo(f.m_color);
//    Resource const * brushRes = cs->fromID(cs->mapInfo(brushInfo));
//    m2::PointF const brushCenter = cs->pipeline(brushRes->m_pipelineID).texture()->mapPixel(brushRes->m_texRect.Center());

//    m2::PointF normal(0.0, 0.0);
//    cs->addTexturedStripStrided(coords , sizeof(m2::PointD),
//                               &normal, 0,
//                               &brushCenter, 0, ARRAY_SIZE(coords),
//                               m_depth, brushRes->m_pipelineID);

//    cs->setDisplayList(0);
//    cs->applyStates();
//    cs->endFrame();
//  }

//  // Create text DL.

//  ASSERT(!text.empty(), ());

//  {
//    m_textDL.reset();
//    m_textDL.reset(cs->createDisplayList());

//    cs->beginFrame();
//    cs->setDisplayList(m_textDL.get());
//    cs->applyVarAlfaStates();

//    strings::UniString uniString = strings::MakeUniString(text);
//    size_t length = uniString.size();
//    buffer_vector<Glyph::Info, 8> infos(length, Glyph::Info());
//    buffer_vector<Resource::Info const *, 8> resInfos(length, NULL);
//    buffer_vector<uint32_t, 8> ids(length, 0);
//    buffer_vector<Resource const *, 8> glyphRes(length, NULL);

//    for (size_t i = 0; i < uniString.size(); ++i)
//    {
//      infos[i] = Glyph::Info(GlyphKey(uniString[i], f.m_size, false, f.m_color),
//                                       controller->GetGlyphCache());

//      resInfos[i] = &infos[i];
//    }

//    if (cs->mapInfo(resInfos.data(), ids.data(), infos.size()))
//    {
//      for (size_t i = 0; i < ids.size(); ++i)
//      {
//        Resource const * res = cs->fromID(ids[i]);
//        glyphRes[i] = res;
//      }

//      int32_t pipelineID = glyphRes[0]->m_pipelineID;
//      shared_ptr<gl::BaseTexture> texture = cs->pipeline(pipelineID).texture();
//      double lengthFromStart = 0.0;

//      buffer_vector<m2::PointF, 48> coords;
//      buffer_vector<m2::PointF, 48> normals;
//      buffer_vector<m2::PointF, 48> texCoords;

//      for (size_t i = 0; i < uniString.size(); ++i)
//      {
//        double baseX = lengthFromStart;
//        coords.push_back(m2::PointD(baseX, 0.0));
//        coords.push_back(m2::PointD(baseX, 0.0));
//        coords.push_back(m2::PointD(baseX, 0.0));

//        coords.push_back(m2::PointD(baseX, 0.0));
//        coords.push_back(m2::PointD(baseX, 0.0));
//        coords.push_back(m2::PointD(baseX, 0.0));

//        m2::RectI resourceRect(glyphRes[i]->m_texRect);
//        resourceRect.Inflate(-1, -1);
//        double w = resourceRect.SizeX();
//        double h = resourceRect.SizeY();
//        lengthFromStart += infos[i].m_metrics.m_xAdvance;

//        normals.push_back(m2::PointF(0.0, 0.0));
//        normals.push_back(m2::PointF(0.0, -h));
//        normals.push_back(m2::PointF(w  , 0.0));

//        normals.push_back(m2::PointF(w  , 0.0));
//        normals.push_back(m2::PointF(0.0, -h));
//        normals.push_back(m2::PointF(w  , -h));

//        texCoords.push_back(texture->mapPixel(m2::PointF(resourceRect.minX(), resourceRect.maxY())));
//        texCoords.push_back(texture->mapPixel(m2::PointF(resourceRect.minX(), resourceRect.minY())));
//        texCoords.push_back(texture->mapPixel(m2::PointF(resourceRect.maxX(), resourceRect.maxY())));

//        texCoords.push_back(texture->mapPixel(m2::PointF(resourceRect.maxX(), resourceRect.maxY())));
//        texCoords.push_back(texture->mapPixel(m2::PointF(resourceRect.minX(), resourceRect.minY())));
//        texCoords.push_back(texture->mapPixel(m2::PointF(resourceRect.maxX(), resourceRect.minY())));
//      }

//      m_textLengthInPx = lengthFromStart;
//      cs->addTexturedListStrided(coords.data(), sizeof(m2::PointF),
//                                 normals.data(), sizeof(m2::PointF),
//                                 texCoords.data(), sizeof(m2::PointF),
//                                 coords.size(), m_depth, pipelineID);
//    }

//    cs->setDisplayList(0);
//    cs->endFrame();
//  }
//}

void Ruler::RulerFrame::Purge()
{
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

///@TODO UVR
//void Ruler::RulerFrame::Draw(OverlayRenderer * r, const math::Matrix<double, 3, 3> & m)
//{
//  ASSERT(m_dl, ());
//  ASSERT(m_textDL, ());

//  UniformsHolder holder;
//  holder.insertValue(ETransparency, GetCurrentAlfa());

//  r->drawDisplayList(m_dl.get(), math::Shift(
//                           math::Scale(m, m2::PointD(m_scale, 1.0)),
//                            m_orgPt), &holder);

//  double const yOffset = -(2 + TextOffsetFromRuler * m_f.GetVisualScale());
//  r->drawDisplayList(m_textDL.get(),
//                     math::Shift(m, m_orgPt + m2::PointF(CacheLength * m_scale - m_textLengthInPx, yOffset)),
//                     &holder);
//}

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
  : m_currentRangeIndex(InvalidUnitValue),
    m_currSystem(0),
    m_framework(p.m_framework)
{
  ///@TODO UVR
  //setIsVisible(false);
}

void Ruler::AnimateShow()
{
  ///@TODO UVR
//  RulerFrame * frame = GetMainFrame();
//  if (!isVisible() && (!frame->IsAnimActive() || frame->IsHidingAnim()))
//  {
//    setIsVisible(true);
//    frame->ShowAnimate(false);
//    m_framework->Invalidate();
//  }
//  else if (isVisible() && (frame->IsAnimActive() && frame->IsHidingAnim()))
//  {
//    frame->ShowAnimate(false);
//    m_framework->Invalidate();
//  }
}

void Ruler::AnimateHide()
{
  ///@TODO UVR
//  RulerFrame * frame = GetMainFrame();
//  if (isVisible() && (!frame->IsAnimActive() || !frame->IsHidingAnim()))
//  {
//    frame->HideAnimate(true);
//    m_framework->Invalidate();
//  }
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
  }

  if (prevCurrSystem != m_currSystem)
    m_currentRangeIndex = InvalidUnitValue;

  update();
}

void Ruler::UpdateText(const string & text)
{
  ///@TODO UVR
//  RulerFrame * frame = GetMainFrame();
//  if (frame->IsAnimActive() && frame->IsHidingAnim())
//    return;

//  if (frame->IsValid())
//    m_animFrame.reset(new RulerFrame(*frame, bind(&Ruler::AnimFrameAnimEnded, this, _1, _2)));

//  frame->Cache(text, font(EActive));
//  if (isVisible())
//    frame->ShowAnimate(true);
}

void Ruler::MainFrameAnimEnded(bool isVisible, RulerFrame * frame)
{
  ///@TODO UVR
//  setIsVisible(isVisible);
//  ASSERT(GetMainFrame() == frame, ());
}

void Ruler::AnimFrameAnimEnded(bool /*isVisible*/, RulerFrame * frame)
{
  ASSERT_EQUAL(m_animFrame.get(), frame, ());
  m_animFrame.reset();
}

Ruler::RulerFrame * Ruler::GetMainFrame()
{
  ///@TODO UVR
//  if (!m_mainFrame)
//    m_mainFrame.reset(new RulerFrame(*m_framework, bind(&Ruler::MainFrameAnimEnded, this, _1, _2), depth()));
//  return m_mainFrame.get();
  return nullptr;
}

Ruler::RulerFrame * Ruler::GetMainFrame() const
{
  ASSERT(m_mainFrame, ());
  return m_mainFrame.get();
}

void Ruler::purge()
{
  m_currentRangeIndex = InvalidUnitValue;

  m_mainFrame.reset();
  m_animFrame.reset();

  ///@TODO UVR
  //setIsVisible(false);
}

void Ruler::update()
{
  ///@TODO UVR
//  double const k = visualScale();

//  ScreenBase const & screen = m_framework->GetNavigator().Screen();

//  int const rulerHeight = my::rounds(RulerHeight * k);
//  int const minPxWidth = my::rounds(MinPixelWidth * k);

//  // pivot() here is the down right point of the ruler.
//  // Get global points of ruler and distance according to minPxWidth.

//  m2::PointD pt1 = screen.PtoG(pivot());
//  m2::PointD pt0 = screen.PtoG(pivot() - m2::PointD(minPxWidth, 0));

//  double const distanceInMetres = MercatorBounds::DistanceOnEarth(pt0, pt1);

//  // convert metres to units for calculating m_metresDiff
//  double metersDiff = CalcMetresDiff(distanceInMetres);

//  bool const higherThanMax = metersDiff > MaxMetersWidth;
//  bool const lessThanMin = metersDiff < MinMetersWidth;

//  // Calculate width of the ruler in pixels.
//  double scalerWidthInPx = minPxWidth;

//  if (higherThanMax)
//    scalerWidthInPx = minPxWidth * 3 / 2;
//  else if (!lessThanMin)
//  {
//    // Here we need to convert metres to pixels according to angle
//    // (in global coordinates) of the ruler.

//    double const a = ang::AngleTo(pt1, pt0);
//    pt0 = MercatorBounds::GetSmPoint(pt1, cos(a) * metersDiff, sin(a) * metersDiff);

//    scalerWidthInPx = my::rounds(pivot().Length(screen.GtoP(pt0)));
//  }

//  m2::PointD orgPt = pivot() + m2::PointD(-scalerWidthInPx / 2, rulerHeight / 2);

//  if (position() & EPosLeft)
//    orgPt.x -= scalerWidthInPx / 2;

//  if (position() & EPosRight)
//    orgPt.x += scalerWidthInPx / 2;

//  if (position() & EPosAbove)
//    orgPt.y -= rulerHeight / 2;

//  if (position() & EPosUnder)
//    orgPt.y += rulerHeight / 2;

//  RulerFrame * frame = GetMainFrame();
//  frame->SetScale(scalerWidthInPx / CacheLength);
//  frame->SetOrgPoint(orgPt);
}

m2::RectD Ruler::GetBoundRect() const
{
  ///@TODO UVR
//  FontDesc const & f = font(EActive);
//  RulerFrame * frame = GetMainFrame();
//  m2::PointD const org = frame->GetOrgPoint();
//  m2::PointD const size = m2::PointD(CacheLength * frame->GetScale(), f.m_size * 2);
//  return m2::RectD(org - m2::PointD(size.x, 0.0), org + m2::PointD(0.0, size.y));
  return m2::RectD();
}

void Ruler::cache()
{
  (void)GetMainFrame();
  update();
}

///@TODO UVR
//void Ruler::draw(OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const
//{
//  if (isVisible())
//  {
//    checkDirtyLayout();

//    RulerFrame * frame = GetMainFrame();
//    frame->Draw(s, m);
//    if (m_animFrame)
//      m_animFrame->Draw(s, m);
//  }
//}

int Ruler::GetTextOffsetFromLine() const
{
  return TextOffsetFromRuler;
}
