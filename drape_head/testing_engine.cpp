#include "drape_head/testing_engine.hpp"

#include "coding/file_reader.hpp"
#include "platform/platform.hpp"

#include "drape_frontend/gui/gui_text.hpp"

#include "drape_frontend/visual_params.hpp"
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/text_shape.hpp"
#include "drape_frontend/path_text_shape.hpp"
#include "drape_frontend/path_symbol_shape.hpp"
#include "drape_frontend/route_shape.hpp"
#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/circle_shape.hpp"
#include "drape_frontend/poi_symbol_shape.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/glsl_types.hpp"
#include "drape/vertex_array_buffer.hpp"
#include "drape/shader_def.hpp"
#include "drape/overlay_tree.hpp"
#include "drape/stipple_pen_resource.hpp"
#include "drape/utils/projection.hpp"

#include "geometry/transformations.hpp"

#include "base/stl_add.hpp"
#include "base/timer.hpp"

#include "std/bind.hpp"
#include "std/function.hpp"
#include "std/vector.hpp"

#include "3party/jansson/myjansson.hpp"

namespace df
{

class DummyLabel
{
public:
  DummyLabel(m2::PointF const & base, string const & text, dp::Anchor anchor, dp::FontDecl const & font)
    : m_base(base)
    , m_text(text)
    , m_anchor(anchor)
    , m_font(font)
  {
  }

  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
  {
    gui::StaticLabel::LabelResult result;
    gui::StaticLabel::CacheStaticText(m_text, "\n", m_anchor, m_font, textures, result);
    for (gui::StaticLabel::Vertex & v : result.m_buffer)
      v.m_position = glsl::vec3(glsl::ToVec2(m_base), v.m_position.z);

    dp::AttributeProvider provider(1 /* streamCount */, result.m_buffer.size());
    provider.InitStream(0 /* streamIndex */, gui::StaticLabel::Vertex::GetBindingInfo(),
                        make_ref(result.m_buffer.data()));

    batcher->InsertListOfStrip(result.m_state, make_ref(&provider),
                               4 /* vertexStride */);
  }

private:
  m2::PointF m_base;
  string const & m_text;
  dp::Anchor m_anchor;
  dp::FontDecl m_font;
};

class DummyMutableLabel
{
public:
  DummyMutableLabel(m2::PointF const & base, string const & text)
    : m_base(base)
    , m_text(text)
  {
  }

  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
  {
    gui::MutableLabel textCacher(dp::LeftBottom);
    gui::MutableLabel::PrecacheParams p;
    p.m_maxLength = 10;
    p.m_alphabet = m_text;
    p.m_font = dp::FontDecl(dp::Color(0, 0, 0, 255), 14);

    gui::MutableLabel::PrecacheResult staticData;
    textCacher.Precache(p, staticData, textures);

    glsl::vec2 offset = glsl::ToVec2(m_base);
    for (gui::MutableLabel::StaticVertex & v : staticData.m_buffer)
      v.m_position = glsl::vec3(v.m_position.xy() + offset, v.m_position.z);

    gui::MutableLabel::LabelResult dynResult;
    textCacher.SetText(dynResult, m_text);
    ASSERT_EQUAL(staticData.m_buffer.size(), dynResult.m_buffer.size(), ());

    dp::AttributeProvider provider(2, dynResult.m_buffer.size());
    provider.InitStream(0 /* streamIndex */, gui::MutableLabel::StaticVertex::GetBindingInfo(),
                        make_ref(staticData.m_buffer.data()));
    provider.InitStream(1 /* streamIndex */, gui::MutableLabel::DynamicVertex::GetBindingInfo(),
                        make_ref(dynResult.m_buffer.data()));

    batcher->InsertListOfStrip(staticData.m_state, make_ref(&provider), 4);
  }

private:
  m2::PointF m_base;
  string m_text;
};

class DummyStippleElement : public MapShape
{
public:
  DummyStippleElement(m2::PointU const & base)
    : m_base(base)
  {
  }

  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
  {
    dp::TextureManager::TStipplePattern key;
    key.push_back(10);
    key.push_back(3);
    key.push_back(7);
    key.push_back(5);
    key.push_back(5);
    key.push_back(10);
    dp::TextureManager::StippleRegion region;
    textures->GetStippleRegion(key, region);

    m2::RectF const & rect = region.GetTexRect();
    uint32_t length = region.GetMaskPixelLength();

    glsl::vec3 startPos(m_base.x, m_base.y, 0.0f);
    glsl::vec3 endPos = startPos + glsl::vec3(length, 0.0f, 0.0f);

    gpu::SolidTexturingVertex vertexes[4] =
    {
      gpu::SolidTexturingVertex(startPos, glsl::vec2(0.0f,  1.0f), glsl::ToVec2(rect.LeftBottom())),
      gpu::SolidTexturingVertex(startPos, glsl::vec2(0.0f, -1.0f), glsl::ToVec2(rect.LeftTop())),
      gpu::SolidTexturingVertex(endPos, glsl::vec2(0.0f, 1.0f), glsl::ToVec2(rect.RightBottom())),
      gpu::SolidTexturingVertex(endPos, glsl::vec2(0.0f, -1.0f), glsl::ToVec2(rect.RightTop()))
    };


    dp::AttributeProvider provider(1, 4);
    provider.InitStream(0, gpu::SolidTexturingVertex::GetBindingInfo(), make_ref(vertexes));

    dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::GeometryLayer);
    state.SetColorTexture(region.GetTexture());

    batcher->InsertTriangleStrip(state, make_ref(&provider));
  }

private:
  m2::PointU m_base;
};

class DummyColorElement : public MapShape
{
public:
  DummyColorElement() { }

  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
  {
    dp::TextureManager::ColorRegion region;
    textures->GetColorRegion(dp::Color(rand() % 256, rand() % 256, rand() % 256, 255), region);

    m2::RectF const & rect = region.GetTexRect();

    glsl::vec3 const basePoint(900.0f, 700.0f, 0.0f);
    float const halfSize = 12.0f;
    glsl::vec2 texCoord = glsl::ToVec2(rect.Center());
    gpu::SolidTexturingVertex vertexes[4] =
    {
      gpu::SolidTexturingVertex(basePoint, glsl::vec2(-halfSize, halfSize), texCoord),
      gpu::SolidTexturingVertex(basePoint, glsl::vec2(-halfSize, -halfSize), texCoord),
      gpu::SolidTexturingVertex(basePoint, glsl::vec2(halfSize, halfSize), texCoord),
      gpu::SolidTexturingVertex(basePoint, glsl::vec2(halfSize, -halfSize), texCoord)
    };

    dp::AttributeProvider provider(1, 4);
    provider.InitStream(0, gpu::SolidTexturingVertex::GetBindingInfo(), make_ref(vertexes));

    dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::GeometryLayer);
    state.SetColorTexture(region.GetTexture());

    batcher->InsertTriangleStrip(state, make_ref(&provider));
  }
};

class MapShapeFactory
{
  typedef function<MapShape * (json_t *)> TCreateFn;
  typedef map<string, TCreateFn> TCreatorsMap;

public:
  MapShapeFactory()
  {
    m_creators["line"] = bind(&MapShapeFactory::CreateLine, this, _1);
    m_creators["area"] = bind(&MapShapeFactory::CreateArea, this, _1);
    m_creators["circle"] = bind(&MapShapeFactory::CreateCircle, this, _1);
  }

  void CreateShapes(vector<MapShape *> & shapes, json_t * object)
  {
    void * iter = json_object_iter(object);
    while(iter)
    {
      json_t * entry = json_object_iter_value(iter);
      if (entry)
      {
        string const type(json_object_iter_key(iter));
        if (type != "_comment_")
        {
          TCreatorsMap::const_iterator it = m_creators.find(type);
          ASSERT(it != m_creators.end(), ());
          shapes.push_back(it->second(entry));
        }
        iter = json_object_iter_next(object, iter);
      }
    }
  }

private:
  dp::Color ParseColor(json_t * object)
  {
    ASSERT(json_array_size(object) == 4, ());
    int const r = json_integer_value(json_array_get(object, 0));
    int const g = json_integer_value(json_array_get(object, 1));
    int const b = json_integer_value(json_array_get(object, 2));
    int const a = json_integer_value(json_array_get(object, 3));
    return dp::Color(r, g, b, a);
  }

  float ParseCoord(json_t * object)
  {
    if (json_is_real(object))
      return json_real_value(object);
    else if (json_is_integer(object))
      return json_integer_value(object);

    ASSERT(false, ());
    return 0.0f;
  }

  void ParseGeometry(json_t * object, vector<m2::PointD> & points)
  {
    size_t const count = json_array_size(object);
    ASSERT((count & 1) == 0, ());
    points.reserve(count >> 1);
    for (size_t i = 0; i < count; i += 2)
    {
      double const x = ParseCoord(json_array_get(object, i));
      double const y = ParseCoord(json_array_get(object, i + 1));
      points.push_back(m2::PointD(x, y));
    }
  }

  void ParseGeometry(json_t * object, vector<m2::PointF> & points)
  {
    size_t const count = json_array_size(object);
    ASSERT((count & 1) == 0, ());
    points.reserve(count >> 1);
    for (size_t i = 0; i < count; i += 2)
    {
      double const x = ParseCoord(json_array_get(object, i));
      double const y = ParseCoord(json_array_get(object, i + 1));
      points.push_back(m2::PointF(x, y));
    }
  }

  dp::LineJoin ParseJoin(json_t * object)
  {
    return (dp::LineJoin)json_integer_value(object);
  }

  dp::LineCap ParseCap(json_t * object)
  {
    return (dp::LineCap)json_integer_value(object);
  }

  MapShape * CreateLine(json_t * object)
  {
    LineViewParams params;
    params.m_depth = json_real_value(json_object_get(object, "depth"));
    params.m_color = ParseColor(json_object_get(object, "color"));
    params.m_width = json_real_value(json_object_get(object, "width"));
    params.m_join = ParseJoin(json_object_get(object, "join"));
    params.m_cap = ParseCap(json_object_get(object, "cap"));
    params.m_baseGtoPScale = 1.0;

    vector<m2::PointD> points;
    ParseGeometry(json_object_get(object, "geometry"), points);
    return new LineShape(points, params);
  }

  MapShape * CreateArea(json_t * object)
  {
    AreaViewParams params;
    params.m_depth = json_real_value(json_object_get(object, "depth"));
    params.m_color = ParseColor(json_object_get(object, "color"));
    vector<m2::PointF> points;
    ParseGeometry(json_object_get(object, "geometry"), points);

    return new AreaShape(move(points), params);
  }

  MapShape * CreateCircle(json_t * object)
  {
    CircleViewParams params(FeatureID(MwmSet::MwmId(), 0));
    params.m_depth = json_real_value(json_object_get(object, "depth"));
    params.m_color = ParseColor(json_object_get(object, "color"));
    params.m_radius = json_real_value(json_object_get(object, "radius"));
    vector<m2::PointF> point;
    ParseGeometry(json_object_get(object, "geometry"), point);

    return new CircleShape(point[0], params);
  }

private:
  TCreatorsMap m_creators;
};

TestingEngine::TestingEngine(ref_ptr<dp::OGLContextFactory> oglcontextfactory,
                             Viewport const & viewport,
                             double vs)
  : m_contextFactory(oglcontextfactory)
  , m_viewport(viewport)
{
  m_contextFactory->getDrawContext()->makeCurrent();
  df::VisualParams::Init(vs, df::CalculateTileSize(viewport.GetWidth(), viewport.GetHeight()));

  GLFunctions::Init();

  dp::TextureManager::Params params;
  params.m_resPrefix = VisualParams::Instance().GetResourcePostfix();
  params.m_glyphMngParams.m_uniBlocks = "unicode_blocks.txt";
  params.m_glyphMngParams.m_whitelist = "fonts_whitelist.txt";
  params.m_glyphMngParams.m_blacklist = "fonts_blacklist.txt";
  GetPlatform().GetFontNames(params.m_glyphMngParams.m_fonts);

  m_textures = make_unique_dp<dp::TextureManager>();
  m_textures->Init(params);

  m_batcher = make_unique_dp<dp::Batcher>(65000, 65000);
  m_programManager = make_unique_dp<dp::GpuProgramManager>();

  ModelViewInit();
  ProjectionInit();

  m_timerId = startTimer(1000 / 30);
}

TestingEngine::~TestingEngine()
{
  killTimer(m_timerId);
  ClearScene();
  m_textures->Release();
}

void TestingEngine::Draw()
{
  static bool isInitialized = false;
  if (!isInitialized)
  {
    DrawImpl();
    m_batcher->StartSession(bind(&df::TestingEngine::OnFlushData, this, _1, _2));
    DrawRects();
    m_batcher->EndSession();
    m_textures->UpdateDynamicTextures();
    isInitialized = true;
  }

  ModelViewInit();
  m_angle += 0.005;

  dp::OGLContext * context = m_contextFactory->getDrawContext();
  context->setDefaultFramebuffer();

  m_viewport.Apply();
  GLFunctions::glClearColor(0.65f, 0.65f, 0.65f, 1.0f);
  GLFunctions::glClear();
  GLFunctions::glEnable(gl_const::GLDepthTest);

  GLFunctions::glFrontFace(gl_const::GLClockwise);
  GLFunctions::glCullFace(gl_const::GLBack);
  GLFunctions::glEnable(gl_const::GLCullFace);

  TScene::iterator it = m_scene.begin();
  for(; it != m_scene.end(); ++it)
  {
    m_textures->UpdateDynamicTextures();

    dp::GLState const & state = it->first;
    ref_ptr<dp::GpuProgram> prg = m_programManager->GetProgram(state.GetProgramIndex());
    prg->Bind();
    ApplyState(state, prg);
    ApplyUniforms(m_generalUniforms, prg);

    vector<drape_ptr<dp::RenderBucket> > & buckets = it->second;
    dp::OverlayTree tree;
    tree.StartOverlayPlacing(m_modelView, true);
    for (size_t i = 0; i < buckets.size(); ++i)
      buckets[i]->CollectOverlayHandles(make_ref(&tree), false);
    for (size_t i = 0; i < buckets.size(); ++i)
      buckets[i]->Render(m_modelView);
    tree.EndOverlayPlacing();
  }

  context->present();
}

void TestingEngine::Resize(int w, int h)
{
  m_modelView.OnSize(0, 0, w, h);
  m_modelView.SetFromRect(m2::AnyRectD(m2::RectD(0, 0, w, h)));
  m_viewport.SetViewport(0, 0, w, h);
  ModelViewInit();
  ProjectionInit();
  Draw();
}

void TestingEngine::timerEvent(QTimerEvent * e)
{
  if (e->timerId() == m_timerId)
    Draw();
}

void TestingEngine::DrawImpl()
{
  m_generalUniforms.SetFloatValue("u_opacity", 1.0f);
  m_generalUniforms.SetFloatValue("u_color", 0.0f, 0.0f, 1.0f, 0.7f);
  m_generalUniforms.SetFloatValue("u_halfWidth", 15.0f, 15.0f * m_modelView.GetScale());

  m_generalUniforms.SetFloatValue("u_clipLength", 8.0f);

  dp::Batcher::TFlushFn flushFn = bind(&df::TestingEngine::OnFlushData, this, _1, _2);
  m_batcher->StartSession(flushFn);
  dp::FontDecl fd;
  fd.m_color = dp::Color::Black();
  fd.m_outlineColor = dp::Color::White();
  fd.m_size = 32.0f;
  dp::FontDecl auxFd;
  auxFd.m_color = dp::Color(0, 80, 240, 255);
  auxFd.m_outlineColor = dp::Color::Transparent();
  auxFd.m_size = 20.0f;

  TextViewParams params;
  params.m_featureID = FeatureID(MwmSet::MwmId(), 567);
  params.m_depth = 10.0f;
  params.m_anchor = dp::Center;
  params.m_primaryText = "People's republic of China";
  params.m_primaryTextFont = fd;
  params.m_secondaryTextFont = auxFd;
  params.m_secondaryText = "Народная Китайская республика";
  params.m_primaryOffset = m2::PointF(0, 0);
  params.m_secondaryOffset = m2::PointF(0, 0);
  TextShape sh1(m2::PointF(82.277071f, 46.9271164f), params);
  sh1.Draw(make_ref(m_batcher), make_ref(m_textures));

  vector<m2::PointD> path;
  path.push_back(m2::PointD(92.277071f, 50.9271164f));
  path.push_back(m2::PointD(98.277071f, 50.9271164f));
  path.push_back(m2::PointD(106.277071f, 48.9271164f));

  m2::SharedSpline spline(path);
  PathTextViewParams ptvp;
  ptvp.m_baseGtoPScale = 1.0f / m_modelView.GetScale();
  ptvp.m_depth = 100.0f;
  ptvp.m_text = "Some text";
  ptvp.m_textFont = dp::FontDecl(dp::Color::Black(), 40, dp::Color::Red());

  PathTextShape(spline, ptvp).Draw(make_ref(m_batcher), make_ref(m_textures));
  LineViewParams lvp;
  lvp.m_baseGtoPScale = ptvp.m_baseGtoPScale;
  lvp.m_depth = 90.0f;
  lvp.m_cap = dp::SquareCap;
  lvp.m_color = dp::Color::Red();
  lvp.m_width = 16.0f;
  lvp.m_join = dp::BevelJoin;
  LineShape(spline, lvp).Draw(make_ref(m_batcher), make_ref(m_textures));

  {
    PathSymbolViewParams p;
    p.m_depth = 95.0f;
    p.m_baseGtoPScale = ptvp.m_baseGtoPScale;
    p.m_offset = 0.0f;
    p.m_step = 60.0f;
    p.m_symbolName = "swimming";
    PathSymbolShape(spline, p).Draw(make_ref(m_batcher), make_ref(m_textures));
  }

  {
    vector<m2::PointD> path1;
    path1.push_back(m2::PointD(92.277071f, 47.9271164f));
    path1.push_back(m2::PointD(92.277071f, 45.9271164f));
    path1.push_back(m2::PointD(98.277071f, 45.9271164f));
    path1.push_back(m2::PointD(98.277071f, 40.9271164f));
    path1.push_back(m2::PointD(100.277071f, 42.9271164f));
    path1.push_back(m2::PointD(101.277071f, 49.9271164f));
    path1.push_back(m2::PointD(102.277071f, 40.9271164f));
    path1.push_back(m2::PointD(105.277071f, 40.9271164f));
    path1.push_back(m2::PointD(101.277071f, 35.9271164f));
    path1.push_back(m2::PointD(90.277071f, 35.9271164f));
    path1.push_back(m2::PointD(89.277071f, 33.9271164f));
    m2::SharedSpline spl1(path1);

    lvp.m_pattern = dp::TextureManager::TStipplePattern{8, 8};
    lvp.m_join = dp::RoundJoin;
    lvp.m_cap = dp::RoundCap;
    lvp.m_color = dp::Color::Black();
    LineShape(spl1, lvp).Draw(make_ref(m_batcher), make_ref(m_textures));
  }

  {
    vector<m2::PointF> trg{ m2::PointD(110.0f, 30.0f), m2::PointD(112.0f, 30.0f), m2::PointD(112.0f, 28.0f),
                            m2::PointD(110.0f, 30.0f), m2::PointD(112.0f, 28.0f), m2::PointD(110.0f, 28.0f) };
    AreaViewParams p;
    p.m_color = dp::Color::White();
    p.m_depth = 0.0f;
    AreaShape(move(trg), p).Draw(make_ref(m_batcher), make_ref(m_textures));
  }
  m_batcher->EndSession();

  m_batcher->StartSession(flushFn);
  {
    vector<m2::PointD> path;
    path.push_back(m2::PointD(120.0f, 30.0f));
    path.push_back(m2::PointD(125.0f, 30.0f));
    m2::SharedSpline spl(path);

    LineViewParams lvpl = lvp;
    lvpl.m_pattern.clear();
    lvpl.m_depth = -10.0f;
    lvpl.m_width = 2.0f;
    LineShape(spl, lvpl).Draw(make_ref(m_batcher), make_ref(m_textures));

    DummyMutableLabel tt(m2::PointF(120.0f, 30.0f), "200 km");
    tt.Draw(make_ref(m_batcher), make_ref(m_textures));
  }
  m_batcher->EndSession();

  m_batcher->StartSession(flushFn);
  {
    {
      vector<m2::PointD> path;
      path.push_back(m2::PointD(110.0f, 25.0f));
      path.push_back(m2::PointD(130.0f, 25.0f));
      m2::SharedSpline spl(path);

      LineViewParams lvpl = lvp;
      lvpl.m_pattern.clear();
      lvpl.m_depth = -10.0f;
      lvpl.m_width = 2.0f;
      LineShape(spl, lvpl).Draw(make_ref(m_batcher), make_ref(m_textures));
    }

    dp::FontDecl font(dp::Color::Black(), 14);

    DummyLabel(m2::PointF(110.0f, 25.0f), "Top\nText", dp::LeftTop, font)
        .Draw(make_ref(m_batcher), make_ref(m_textures));
    DummyLabel(m2::PointF(120.0f, 25.0f), "Center\nText", dp::Center, font)
        .Draw(make_ref(m_batcher), make_ref(m_textures));
    DummyLabel(m2::PointF(130.0f, 25.0f), "Bottom\nText", dp::RightBottom, font)
        .Draw(make_ref(m_batcher), make_ref(m_textures));
  }
  m_batcher->EndSession();
}

void TestingEngine::DrawRects()
{
  LineViewParams lvp;
  lvp.m_baseGtoPScale = m_modelView.GetScale();
  lvp.m_depth = 0.0f;
  lvp.m_cap = dp::RoundCap;
  lvp.m_join = dp::RoundJoin;
  lvp.m_color = dp::Color::Red();
  lvp.m_width = 3.0f;

  auto drawRectFn = [&lvp, this](m2::RectD const & r)
  {
    if ((r.LeftBottom() - r.RightTop()).IsAlmostZero())
      return;

    vector<m2::PointD> path
    {
      m_modelView.PtoG(r.LeftBottom()),
      m_modelView.PtoG(r.LeftTop()),
      m_modelView.PtoG(r.RightTop()),
      m_modelView.PtoG(r.RightBottom()),
      m_modelView.PtoG(r.LeftBottom())
    };

    m2::SharedSpline spline(path);
    LineShape(spline, lvp).Draw(make_ref(m_batcher), make_ref(m_textures));
  };

  for (m2::RectD const & r : m_boundRects)
    drawRectFn(r);

  lvp.m_color = dp::Color::White();
  lvp.m_depth = 10.0f;
  for (dp::OverlayHandle::Rects const & rr : m_rects)
  {
    for (m2::RectF const & r : rr)
      drawRectFn(m2::RectD(r));
  }
}

void TestingEngine::ModelViewInit()
{
  math::Matrix<double, 3, 3> m
  {   34.1554f,      0.0f, 0.0f,
          0.0f, -34.1554f, 0.0f,
     -2639.46f,  1800.99f, 1.0f};

//  math::Matrix<double, 3, 3> m = math::Inverse(math::Shift(
//                                                 math::Rotate(
//                                                   math::Scale(
//                                                     math::Shift(math::Identity<double, 3>(),
//                                                                 -m_modelView.PixelRect().Center()),
//                                                               1.0f /34.1554f, 1.0f / -34.1554f),
//                                                   m_angle),
//                                                 92.277071f, 50.9271164f));

  m_modelView.SetGtoPMatrix(m);
  m = m_modelView.GtoPMatrix();

  math::Matrix<float, 4, 4> mv;

  /// preparing ModelView matrix

  mv(0, 0) = m(0, 0); mv(0, 1) = m(1, 0); mv(0, 2) = 0; mv(0, 3) = m(2, 0);
  mv(1, 0) = m(0, 1); mv(1, 1) = m(1, 1); mv(1, 2) = 0; mv(1, 3) = m(2, 1);
  mv(2, 0) = 0;       mv(2, 1) = 0;       mv(2, 2) = 1; mv(2, 3) = 0;
  mv(3, 0) = m(0, 2); mv(3, 1) = m(1, 2); mv(3, 2) = 0; mv(3, 3) = m(2, 2);

  m_generalUniforms.SetMatrix4x4Value("modelView", mv.m_data);
}

void TestingEngine::ProjectionInit()
{
  float const left = m_viewport.GetX0();
  float const right = left + m_viewport.GetWidth();
  float const bottom = m_viewport.GetY0() + m_viewport.GetHeight();
  float const top = m_viewport.GetY0();

  array<float, 16> m;
  dp::MakeProjection(m, left, right, bottom, top);
  m_generalUniforms.SetMatrix4x4Value("projection", m.data());
}

void TestingEngine::OnFlushData(dp::GLState const & state, drape_ptr<dp::RenderBucket> && vao)
{
  drape_ptr<dp::RenderBucket> bucket = move(vao);
  bucket->GetBuffer()->Build(m_programManager->GetProgram(state.GetProgramIndex()));
  m_scene[state].push_back(move(bucket));
  for (size_t i = 0; i < m_scene[state].back()->GetOverlayHandlesCount(); ++i)
  {
    ref_ptr<dp::OverlayHandle> handle = m_scene[state].back()->GetOverlayHandle(i);
    handle->Update(m_modelView);
    if (handle->IsValid())
    {
      m_boundRects.push_back(handle->GetPixelRect(m_modelView));
      m_rects.resize(m_rects.size() + 1);
      handle->GetPixelShape(m_modelView, m_rects.back());
    }
  };
}

void TestingEngine::ClearScene()
{
  m_scene.clear();
}

} // namespace df
