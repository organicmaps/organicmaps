#include "testing_engine.hpp"

#include "../coding/file_reader.hpp"
#include "../platform/platform.hpp"

#include "../drape/vertex_array_buffer.hpp"
#include "../drape/shader_def.hpp"
#include "../drape/overlay_tree.hpp"
#include "../drape/stipple_pen_resource.hpp"

#include "../drape_frontend/visual_params.hpp"
#include "../drape_frontend/line_shape.hpp"
#include "../drape_frontend/text_shape.hpp"
#include "../drape_frontend/path_text_shape.hpp"
#include "../drape_frontend/path_symbol_shape.hpp"
#include "../drape_frontend/area_shape.hpp"
#include "../drape_frontend/circle_shape.hpp"

#include "../geometry/transformations.hpp"

#include "../base/stl_add.hpp"
#include "../base/timer.hpp"

#include "../std/bind.hpp"
#include "../std/function.hpp"
#include "../std/vector.hpp"

#include "../../3party/jansson/myjansson.hpp"

namespace df
{

class DummyStippleElement : public MapShape
{
public:
  DummyStippleElement(m2::PointU const & base)
    : m_base(base)
  {
  }

  void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
  {
    dp::StipplePenKey key;
    key.m_pattern.push_back(10);
    key.m_pattern.push_back(3);
    key.m_pattern.push_back(7);
    key.m_pattern.push_back(5);
    key.m_pattern.push_back(5);
    key.m_pattern.push_back(10);
    dp::TextureSetHolder::StippleRegion region;
    textures->GetStippleRegion(key, region);

    m2::RectF const & rect = region.GetTexRect();
    float texIndex = static_cast<float>(region.GetTextureNode().m_textureOffset);

    uint32_t length = region.GetTemplateLength();
    m2::PointF positions[4] =
    {
      m_base, m_base,
      m_base + m2::PointF(length, 0.0), m_base + m2::PointF(length, 0.0)
    };

    m2::PointF normals[4] =
    {
      m2::PointF(0.0, 1.0), m2::PointF(0.0, -1.0),
      m2::PointF(0.0, 1.0), m2::PointF(0.0, -1.0)
    };

    glsl_types::vec3 texCoord[4] =
    {
      glsl_types::vec3(rect.minX(), rect.minY(), texIndex),
      glsl_types::vec3(rect.minX(), rect.maxY(), texIndex),
      glsl_types::vec3(rect.maxX(), rect.minY(), texIndex),
      glsl_types::vec3(rect.maxX(), rect.maxY(), texIndex)
    };

    dp::AttributeProvider provider(3, 4);
    {
      dp::BindingInfo info(1);
      dp::BindingDecl & decl = info.GetBindingDecl(0);
      decl.m_attributeName = "a_position";
      decl.m_componentCount = 2;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(0, info, dp::MakeStackRefPointer<void>(positions));
    }

    {
      dp::BindingInfo info(1);
      dp::BindingDecl & decl = info.GetBindingDecl(0);
      decl.m_attributeName = "a_normal";
      decl.m_componentCount = 2;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(1, info, dp::MakeStackRefPointer<void>(normals));
    }

    {
      dp::BindingInfo info(1);
      dp::BindingDecl & decl = info.GetBindingDecl(0);
      decl.m_attributeName = "a_texCoords";
      decl.m_componentCount = 3;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(2, info, dp::MakeStackRefPointer<void>(texCoord));
    }

    dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::GeometryLayer);
    state.SetTextureSet(region.GetTextureNode().m_textureSet);
    state.SetBlending(dp::Blending(true));

    batcher->InsertTriangleStrip(state, dp::MakeStackRefPointer(&provider));
  }

private:
  m2::PointU m_base;
};

class SquareHandle : public dp::OverlayHandle
{
public:
  static const uint8_t NormalAttributeID = 1;
  SquareHandle(vector<m2::PointF> const & formingVector)
    : OverlayHandle(FeatureID(), dp::Center, 0.0f)
    , m_vectors(formingVector)
  {
    SetIsVisible(true);
  }

  virtual m2::RectD GetPixelRect(ScreenBase const & screen) const { return m2::RectD(); }
  void GetPixelShape(ScreenBase const & screen, Rects & rects) const
  {
    m2::RectD rd = GetPixelRect(screen);
    rects.push_back(m2::RectF(rd.minX(), rd.minY(), rd.maxX(), rd.maxY()));
  }

  virtual void GetAttributeMutation(dp::RefPointer<dp::AttributeBufferMutator> mutator, ScreenBase const & screen) const
  {
    static const my::Timer timer;
    double const angle = timer.ElapsedSeconds();

    math::Matrix<double, 3, 3> const m = math::Rotate(math::Identity<double, 3>(), angle);

    vector<m2::PointF> data(4);
    for (size_t i = 0; i < m_vectors.size(); ++i)
      data[i] = m_vectors[i] * m;

    TOffsetNode const & node = GetOffsetNode(NormalAttributeID);
    dp::MutateNode mutateNode;
    mutateNode.m_region = node.second;
    mutateNode.m_data = dp::MakeStackRefPointer<void>(&data[0]);
    mutator->AddMutation(node.first, mutateNode);
  }

private:
  vector<m2::PointF> m_vectors;
};

class SquareShape : public MapShape
{
public:
  SquareShape(m2::PointF const & center, float radius)
    : m_center(center)
    , m_radius(radius)
  {
  }

  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
  {
    vector<m2::PointF> vertexes(4, m_center);

    vector<m2::PointF> formingVectors(4);
    formingVectors[0] = m2::PointF(-m_radius,  m_radius);
    formingVectors[1] = m2::PointF(-m_radius, -m_radius);
    formingVectors[2] = m2::PointF( m_radius,  m_radius);
    formingVectors[3] = m2::PointF( m_radius, -m_radius);

    dp::AttributeProvider provider(2, 4);
    {
      dp::BindingInfo info(1);
      dp::BindingDecl & decl = info.GetBindingDecl(0);
      decl.m_attributeName = "a_position";
      decl.m_componentCount = 2;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(0, info, dp::MakeStackRefPointer<void>(&vertexes[0]));
    }
    {
      dp::BindingInfo info(1, SquareHandle::NormalAttributeID);
      dp::BindingDecl & decl = info.GetBindingDecl(0);
      decl.m_attributeName = "a_normal";
      decl.m_componentCount = 2;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(1, info, dp::MakeStackRefPointer<void>(&formingVectors[0]));
    }

    dp::GLState state(gpu::TEST_DYN_ATTR_PROGRAM, dp::GLState::GeometryLayer);
    state.SetColor(dp::Color(150, 130, 120, 255));

    dp::OverlayHandle * handle = new SquareHandle(formingVectors);

    batcher->InsertTriangleStrip(state, dp::MakeStackRefPointer<dp::AttributeProvider>(&provider), dp::MovePointer(handle));
  }

private:
  m2::PointF m_center;
  float m_radius;
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
    m_creators["dyn_square"] = bind(&MapShapeFactory::CreateDynSquare, this, _1);
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

    vector<m2::PointF> points;
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

    return new AreaShape(points, params);
  }

  MapShape * CreateDynSquare(json_t * object)
  {
    float radius = json_real_value(json_object_get(object, "radius"));
    vector<m2::PointF> point;
    ParseGeometry(json_object_get(object, "geometry"), point);
    return new SquareShape(point[0], radius);
  }

  MapShape * CreateCircle(json_t * object)
  {
    CircleViewParams params(FeatureID(-1, 0));
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

TestingEngine::TestingEngine(dp::RefPointer<dp::OGLContextFactory> oglcontextfactory,
                             double vs, df::Viewport const & viewport)
  : m_contextFactory(oglcontextfactory)
  , m_viewport(viewport)
{
  GLFunctions::Init();
  df::VisualParams::Init(vs, df::CalculateTileSize(viewport.GetWidth(), viewport.GetHeight()));
  m_contextFactory->getDrawContext()->makeCurrent();

  m_textures.Reset(new dp::TextureManager());
  m_textures->Init(df::VisualParams::Instance().GetResourcePostfix());
  m_batcher.Reset(new dp::Batcher());
  m_programManager.Reset(new dp::GpuProgramManager());

  ModelViewInit();
  ProjectionInit();

  m_timerId = startTimer(1000 / 30);
}

TestingEngine::~TestingEngine()
{
  killTimer(m_timerId);
  ClearScene();
  m_batcher.Destroy();
  m_textures->Release();
  m_textures.Destroy();
  m_programManager.Destroy();
}

void TestingEngine::Draw()
{
  static bool isInitialized = false;
  if (!isInitialized)
  {
    m_batcher->StartSession(bind(&df::TestingEngine::OnFlushData, this, _1, _2));
    DrawImpl();
    m_batcher->EndSession();
    m_textures->UpdateDynamicTextures();
    isInitialized = true;
  }

  dp::OGLContext * context = m_contextFactory->getDrawContext();
  context->setDefaultFramebuffer();

  m_viewport.Apply();
  GLFunctions::glClearColor(0.65f, 0.65f, 0.65f, 1.0f);
  GLFunctions::glClear();

  TScene::iterator it = m_scene.begin();
  for(; it != m_scene.end(); ++it)
  {
    dp::GLState const & state = it->first;
    dp::RefPointer<dp::GpuProgram> prg = m_programManager->GetProgram(state.GetProgramIndex());
    prg->Bind();
    dp::TextureSetBinder binder(m_textures.GetRefPointer());
    ApplyState(state, prg, dp::MakeStackRefPointer<dp::TextureSetController>(&binder));
    ApplyUniforms(m_generalUniforms, prg);

    vector<dp::MasterPointer<dp::RenderBucket> > & buckets = it->second;
    dp::OverlayTree tree;
    tree.StartOverlayPlacing(m_modelView, true);
    for (size_t i = 0; i < buckets.size(); ++i)
      buckets[i]->CollectOverlayHandles(MakeStackRefPointer(&tree));
    for (size_t i = 0; i < buckets.size(); ++i)
      buckets[i]->Render(m_modelView);
    tree.EndOverlayPlacing();
  }

  context->present();
}

void TestingEngine::Resize(int w, int h)
{
  m_modelView.OnSize(0, 0, w, h);
  m_viewport.SetViewport(0, 0, w, h);
  ModelViewInit();
  ProjectionInit();
  Draw();
}

void TestingEngine::DragStarted(m2::PointF const & p) {}
void TestingEngine::Drag(m2::PointF const & p) {}
void TestingEngine::DragEnded(m2::PointF const & p) {}
void TestingEngine::Scale(m2::PointF const & p, double factor) {}

void TestingEngine::timerEvent(QTimerEvent * e)
{
  if (e->timerId() == m_timerId)
    Draw();
}

void TestingEngine::DrawImpl()
{
  ReaderPtr<ModelReader> reader = GetPlatform().GetReader("test_scene.json");
  string jsonString;
  reader.ReadAsString(jsonString);

  vector<MapShape *> shapes;
  try
  {
    my::Json json(jsonString.c_str());
    MapShapeFactory factory;
    factory.CreateShapes(shapes, json.get());
  }
  catch (RootException & e)
  {
    LOG(LCRITICAL, (e.Msg()));
  }

  //for (size_t i = 0; i < shapes.size(); ++i)
    shapes[1]->Draw(m_batcher.GetRefPointer(), m_textures.GetRefPointer());

  DeleteRange(shapes, DeleteFunctor());

  FontDecl fd;
  fd.m_color = dp::Color(200, 80, 240, 255);
  fd.m_needOutline = true;
  fd.m_outlineColor = dp::Color(255, 255, 255, 255);
  fd.m_size = 60.0f;
  FontDecl auxFd;
  auxFd.m_color = dp::Color(0, 80, 240, 255);
  auxFd.m_needOutline = false;
  auxFd.m_outlineColor = dp::Color(0, 255, 0, 255);
  auxFd.m_size = 20.0f;

  TextViewParams params;
  params.m_featureID = FeatureID(23, 567);
  params.m_depth = 10.0f;
  params.m_anchor = dp::Left;
  params.m_primaryOffset = m2::PointF(0,0);
  params.m_primaryText = "hjksdfhjajsdf";
  params.m_primaryTextFont = fd;
  params.m_secondaryTextFont = auxFd;
  params.m_secondaryText = "Small fix bugs";
  TextShape sh1(m2::PointF(200.0f, 300.0f), params);
  //sh1.Draw(m_batcher.GetRefPointer(), m_textures.GetRefPointer());

  params.m_featureID = FeatureID(23, 78);
  params.m_depth = -10.0f;
  params.m_anchor = dp::RightTop;
  //params.m_primaryTextFont.m_needOutline = false;
  TextShape sh2(m2::PointF(250.0f, 250.0f), params);
  //sh2.Draw(m_batcher.GetRefPointer(), m_textures.GetRefPointer());

  vector<m2::PointF> path;

  path.push_back(m2::PointF(200, 650));
  path.push_back(m2::PointF(200, 450));
  for(int i = 16; i >= 0 ; --i)
  {
    float r = 200.0f;
    float x = r * cos((float)i / 32.0f * 2.0f * M_PI) + 800.0f;
    float y = r * sin((float)i / 32.0f * 2.0f * M_PI) + 450.0f;
    path.push_back(m2::PointF(x, y));
  }
  path.push_back(m2::PointF(1600, 450));

  PathTextViewParams params3;
  params3.m_depth = -10.0f;
  params3.m_text = "√2+√3=?----+";
  params3.m_textFont = params.m_primaryTextFont;
  PathTextShape sh3(path, params3, 1);
  //sh3.Draw(m_batcher.GetRefPointer(), m_textures.GetRefPointer());

  PathSymbolViewParams params4;
  params4.m_featureID = FeatureID(23, 78);
  params4.m_depth = 30.0f;
  params4.m_step = 40.0f;
  params4.m_offset = 0.0f;
  params4.m_symbolName = "arrow";
  PathSymbolShape sh4(path, params4, 10);
  //sh4.Draw(m_batcher.GetRefPointer(), m_textures.GetRefPointer());

  DummyStippleElement e(m2::PointU(100, 900));
  //e.Draw(m_batcher.GetRefPointer(), m_textures.GetRefPointer());
}

void TestingEngine::ModelViewInit()
{
  math::Matrix<double, 3, 3> m = math::Shift(
                                        math::Rotate(
                                            math::Scale(math::Identity<double, 3>(), 1.0, 1.0),
                                                        0.0),
                                             0.0, 0.0);

  m_modelView.SetGtoPMatrix(m);

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
  float left = m_viewport.GetX0();
  float right = left + m_viewport.GetWidth();
  float bottom = m_viewport.GetY0();
  float top = bottom + m_viewport.GetHeight();
  float near = -20000.0f;
  float far = 20000.0f;

  float m[4 * 4];
  memset(m, 0, sizeof(m));
  m[0]  = 2.0f / (right - left);
  m[3]  = - (right + left) / (right - left);
  m[5]  = 2.0f / (bottom - top);
  m[7]  = - (bottom + top) / (bottom - top);
  m[10] = -2.0f / (far - near);
  m[11] = - (far + near) / (far - near);
  m[15] = 1.0;

  m_generalUniforms.SetMatrix4x4Value("projection", m);
}

void TestingEngine::OnFlushData(dp::GLState const & state, dp::TransferPointer<dp::RenderBucket> vao)
{
  dp::MasterPointer<dp::RenderBucket> bucket(vao);
  bucket->GetBuffer()->Build(m_programManager->GetProgram(state.GetProgramIndex()));
  m_scene[state].push_back(bucket);
}

void TestingEngine::ClearScene()
{
  TScene::iterator it = m_scene.begin();
  for(; it != m_scene.end(); ++it)
    DeleteRange(it->second, dp::MasterPointerDeleter());
}

} // namespace df
