#include "my_position.hpp"

#include "drape/depth_constants.hpp"
#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/render_bucket.hpp"
#include "drape/shader_def.hpp"

namespace df
{

namespace
{

struct Vertex
{
  Vertex() = default;
  Vertex(glsl::vec2 const & normal, glsl::vec2 const & texCoord)
    : m_normal(normal)
    , m_texCoord(texCoord)
  {
  }

  glsl::vec2 m_normal;
  glsl::vec2 m_texCoord;
};

dp::BindingInfo GetBindingInfo()
{
  dp::BindingInfo info(2);
  dp::BindingDecl & normal = info.GetBindingDecl(0);
  normal.m_attributeName = "a_normal";
  normal.m_componentCount = 2;
  normal.m_componentType = gl_const::GLFloatType;
  normal.m_offset = 0;
  normal.m_stride = sizeof(Vertex);

  dp::BindingDecl & texCoord = info.GetBindingDecl(1);
  texCoord.m_attributeName = "a_colorTexCoords";
  texCoord.m_componentCount = 2;
  texCoord.m_componentType = gl_const::GLFloatType;
  texCoord.m_offset = sizeof(glsl::vec2);
  texCoord.m_stride = sizeof(Vertex);

  return info;
}

} // namespace

MyPosition::MyPosition(ref_ptr<dp::TextureManager> mng)
  : m_position(m2::PointF::Zero())
  , m_azimuth(0.0f)
  , m_accuracy(0.0f)
  , m_showAzimuth(false)
  , m_isRoutingMode(false)
{
  m_parts.resize(4);
  CacheAccuracySector(mng);
  CachePointPosition(mng);
}

void MyPosition::SetPosition(m2::PointF const & pt)
{
  m_position = pt;
}

void MyPosition::SetAzimuth(float azimut)
{
  m_azimuth = azimut;
}

void MyPosition::SetIsValidAzimuth(bool isValid)
{
  m_showAzimuth = isValid;
}

void MyPosition::SetAccuracy(float accuracy)
{
  m_accuracy = accuracy;
}

void MyPosition::SetRoutingMode(bool routingMode)
{
  m_isRoutingMode = routingMode;
}

void MyPosition::RenderAccuracy(ScreenBase const & screen,
                        ref_ptr<dp::GpuProgramManager> mng,
                        dp::UniformValuesStorage const & commonUniforms)
{
  dp::UniformValuesStorage uniforms = commonUniforms;
  m2::PointD accuracyPoint(m_position.x + m_accuracy, m_position.y);
  float pixelAccuracy = (screen.GtoP(accuracyPoint) - screen.GtoP(m_position)).Length();

  uniforms.SetFloatValue("u_position", m_position.x, m_position.y, dp::depth::POSITION_ACCURACY);
  uniforms.SetFloatValue("u_accuracy", pixelAccuracy);
  uniforms.SetFloatValue("u_opacity", 1.0);
  RenderPart(mng, uniforms, MY_POSITION_ACCURACY);
}

void MyPosition::RenderMyPosition(ScreenBase const & screen,
                                  ref_ptr<dp::GpuProgramManager> mng,
                                  dp::UniformValuesStorage const & commonUniforms)
{
  dp::UniformValuesStorage uniforms = commonUniforms;
  uniforms.SetFloatValue("u_position", m_position.x, m_position.y, dp::depth::MY_POSITION_MARK);
  uniforms.SetFloatValue("u_azimut", -(m_azimuth + screen.GetAngle()));
  uniforms.SetFloatValue("u_opacity", 1.0);
  RenderPart(mng, uniforms, (m_showAzimuth == true) ?
             (m_isRoutingMode ? MY_POSITION_ROUTING_ARROW : MY_POSITION_ARROW) : MY_POSITION_POINT);
}

void MyPosition::CacheAccuracySector(ref_ptr<dp::TextureManager> mng)
{
  int const TriangleCount = 40;
  int const VertexCount = 3 * TriangleCount;
  float const etalonSector = math::twicePi / static_cast<double>(TriangleCount);

  dp::TextureManager::ColorRegion color;
  mng->GetColorRegion(dp::Color(0x51, 0xA3, 0xDC, 0x46), color);
  glsl::vec2 colorCoord = glsl::ToVec2(color.GetTexRect().Center());

  buffer_vector<Vertex, TriangleCount> buffer;
  //buffer.emplace_back(glsl::vec2(0.0f, 0.0f), colorCoord);

  glsl::vec2 startNormal(0.0f, 1.0f);

  for (size_t i = 0; i < TriangleCount + 1; ++i)
  {
    glsl::vec2 normal = glsl::rotate(startNormal, i * etalonSector);
    glsl::vec2 nextNormal = glsl::rotate(startNormal, (i + 1) * etalonSector);

    buffer.emplace_back(startNormal, colorCoord);
    buffer.emplace_back(normal, colorCoord);
    buffer.emplace_back(nextNormal, colorCoord);
  }

  dp::GLState state(gpu::ACCURACY_PROGRAM, dp::GLState::OverlayLayer);
  state.SetColorTexture(color.GetTexture());

  {
    dp::Batcher batcher(TriangleCount * dp::Batcher::IndexPerTriangle, VertexCount);
    dp::SessionGuard guard(batcher, [this](dp::GLState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      drape_ptr<dp::RenderBucket> bucket = move(b);
      ASSERT(bucket->GetOverlayHandlesCount() == 0, ());

      m_nodes.emplace_back(state, bucket->MoveBuffer());
      m_parts[MY_POSITION_ACCURACY].second = m_nodes.size() - 1;
    });

    dp::AttributeProvider provider(1 /*stream count*/, VertexCount);
    provider.InitStream(0 /*stream index*/, GetBindingInfo(), make_ref(buffer.data()));

    m_parts[MY_POSITION_ACCURACY].first = batcher.InsertTriangleList(state, make_ref(&provider), nullptr);
    ASSERT(m_parts[MY_POSITION_ACCURACY].first.IsValid(), ());
  }
}

void MyPosition::CachePointPosition(ref_ptr<dp::TextureManager> mng)
{
  dp::TextureManager::SymbolRegion pointSymbol, arrowSymbol, routingArrowSymbol;
  mng->GetSymbolRegion("current-position", pointSymbol);
  mng->GetSymbolRegion("current-position-compas", arrowSymbol);
  mng->GetSymbolRegion("current-routing-compas", routingArrowSymbol);

  m2::RectF const & pointTexRect = pointSymbol.GetTexRect();
  m2::PointF pointHalfSize = m2::PointF(pointSymbol.GetPixelSize()) * 0.5f;

  Vertex pointData[4]=
  {
    { glsl::vec2(-pointHalfSize.x,  pointHalfSize.y), glsl::ToVec2(pointTexRect.LeftTop()) },
    { glsl::vec2(-pointHalfSize.x, -pointHalfSize.y), glsl::ToVec2(pointTexRect.LeftBottom()) },
    { glsl::vec2( pointHalfSize.x,  pointHalfSize.y), glsl::ToVec2(pointTexRect.RightTop()) },
    { glsl::vec2( pointHalfSize.x, -pointHalfSize.y), glsl::ToVec2(pointTexRect.RightBottom())}
  };

  m2::RectF const & arrowTexRect = arrowSymbol.GetTexRect();
  m2::PointF arrowHalfSize = m2::PointF(arrowSymbol.GetPixelSize()) * 0.5f;

  Vertex arrowData[4]=
  {
    { glsl::vec2(-arrowHalfSize.x,  arrowHalfSize.y), glsl::ToVec2(arrowTexRect.LeftTop()) },
    { glsl::vec2(-arrowHalfSize.x, -arrowHalfSize.y), glsl::ToVec2(arrowTexRect.LeftBottom()) },
    { glsl::vec2( arrowHalfSize.x,  arrowHalfSize.y), glsl::ToVec2(arrowTexRect.RightTop()) },
    { glsl::vec2( arrowHalfSize.x, -arrowHalfSize.y), glsl::ToVec2(arrowTexRect.RightBottom())}
  };

  m2::RectF const & routingArrowTexRect = routingArrowSymbol.GetTexRect();
  m2::PointF routingArrowHalfSize = m2::PointF(routingArrowSymbol.GetPixelSize()) * 0.5f;

  Vertex routingArrowData[4]=
  {
    { glsl::vec2(-routingArrowHalfSize.x,  routingArrowHalfSize.y), glsl::ToVec2(routingArrowTexRect.LeftTop()) },
    { glsl::vec2(-routingArrowHalfSize.x, -routingArrowHalfSize.y), glsl::ToVec2(routingArrowTexRect.LeftBottom()) },
    { glsl::vec2( routingArrowHalfSize.x,  routingArrowHalfSize.y), glsl::ToVec2(routingArrowTexRect.RightTop()) },
    { glsl::vec2( routingArrowHalfSize.x, -routingArrowHalfSize.y), glsl::ToVec2(routingArrowTexRect.RightBottom())}
  };

  ASSERT(pointSymbol.GetTexture() == arrowSymbol.GetTexture(), ());
  ASSERT(pointSymbol.GetTexture() == routingArrowSymbol.GetTexture(), ());
  dp::GLState state(gpu::MY_POSITION_PROGRAM, dp::GLState::OverlayLayer);
  state.SetColorTexture(pointSymbol.GetTexture());

  {
    dp::Batcher batcher(3 * dp::Batcher::IndexPerQuad, 3 * dp::Batcher::VertexPerQuad);
    dp::SessionGuard guard(batcher, [this](dp::GLState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      drape_ptr<dp::RenderBucket> bucket = move(b);
      ASSERT(bucket->GetOverlayHandlesCount() == 0, ());

      m_nodes.emplace_back(state, bucket->MoveBuffer());
    });

    dp::AttributeProvider pointProvider(1 /*stream count*/, dp::Batcher::VertexPerQuad);
    pointProvider.InitStream(0 /*stream index*/, GetBindingInfo(), make_ref(pointData));

    dp::AttributeProvider arrowProvider(1 /*stream count*/, dp::Batcher::VertexPerQuad);
    arrowProvider.InitStream(0 /*stream index*/, GetBindingInfo(), make_ref(arrowData));

    dp::AttributeProvider routingArrowProvider(1 /*stream count*/, dp::Batcher::VertexPerQuad);
    routingArrowProvider.InitStream(0 /*stream index*/, GetBindingInfo(), make_ref(routingArrowData));

    m_parts[MY_POSITION_POINT].second = m_nodes.size();
    m_parts[MY_POSITION_ARROW].second = m_nodes.size();
    m_parts[MY_POSITION_ROUTING_ARROW].second = m_nodes.size();

    m_parts[MY_POSITION_POINT].first = batcher.InsertTriangleStrip(state, make_ref(&pointProvider), nullptr);
    ASSERT(m_parts[MY_POSITION_POINT].first.IsValid(), ());

    m_parts[MY_POSITION_ARROW].first = batcher.InsertTriangleStrip(state, make_ref(&arrowProvider), nullptr);
    ASSERT(m_parts[MY_POSITION_ARROW].first.IsValid(), ());

    m_parts[MY_POSITION_ROUTING_ARROW].first = batcher.InsertTriangleStrip(state, make_ref(&routingArrowProvider), nullptr);
    ASSERT(m_parts[MY_POSITION_ROUTING_ARROW].first.IsValid(), ());
  }
}

void MyPosition::RenderPart(ref_ptr<dp::GpuProgramManager> mng,
                            dp::UniformValuesStorage const & uniforms,
                            MyPosition::EMyPositionPart part)
{
  TPart const & accuracy = m_parts[part];
  RenderNode & node = m_nodes[accuracy.second];
  node.Render(mng, uniforms, accuracy.first);
}

}
