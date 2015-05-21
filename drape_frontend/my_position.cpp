#include "my_position.hpp"

#include "drape/batcher.hpp"
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
  , m_azimut(0.0f)
  , m_accuracy(0.0f)
  , m_showAzimut(false)
{
  m_parts.resize(3);
  CacheAccuracySector(mng);
  CachePointPosition(mng);
}

void MyPosition::SetPosition(m2::PointF const & pt)
{
  m_position = pt;
}

void MyPosition::SetAzimut(float azimut)
{
  m_azimut = azimut;
}

void MyPosition::SetIsValidAzimut(bool isValid)
{
  m_showAzimut = isValid;
}

void MyPosition::SetAccuracy(float accuracy)
{
  m_accuracy = accuracy;
}

void MyPosition::Render(ScreenBase const & screen,
                        ref_ptr<dp::GpuProgramManager> mng,
                        dp::UniformValuesStorage const & commonUniforms)
{
  dp::UniformValuesStorage uniforms = commonUniforms;

  {
    m2::PointD accuracyPoint(m_position.x + m_accuracy, m_position.y);
    float pixelAccuracy = (screen.GtoP(accuracyPoint) - screen.GtoP(m_position)).Length();
    dp::UniformValuesStorage accuracyUniforms = uniforms;
    accuracyUniforms.SetFloatValue("u_position", m_position.x, m_position.y, dp::depth::POSITION_ACCURACY);
    accuracyUniforms.SetFloatValue("u_accuracy", pixelAccuracy);
    RenderPart(mng, accuracyUniforms, MY_POSITION_ACCURACY);
  }

  {
    dp::UniformValuesStorage arrowUniforms = uniforms;
    arrowUniforms.SetFloatValue("u_position", m_position.x, m_position.y, dp::depth::MY_POSITION_MARK);
    arrowUniforms.SetFloatValue("u_azimut", -(m_azimut + screen.GetAngle()));
    RenderPart(mng, arrowUniforms, (m_showAzimut == true) ? MY_POSITION_ARROW : MY_POSITION_POINT);
  }
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
  dp::TextureManager::SymbolRegion pointSymbol, arrowSymbol;
  mng->GetSymbolRegion("current-position", pointSymbol);
  mng->GetSymbolRegion("current-position-compas", arrowSymbol);

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

  ASSERT(pointSymbol.GetTexture() == arrowSymbol.GetTexture(), ());
  dp::GLState state(gpu::MY_POSITION_PROGRAM, dp::GLState::OverlayLayer);
  state.SetColorTexture(pointSymbol.GetTexture());

  {
    dp::Batcher batcher(2 * dp::Batcher::IndexPerQuad, 2 * dp::Batcher::VertexPerQuad);
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

    m_parts[MY_POSITION_POINT].second = m_nodes.size();
    m_parts[MY_POSITION_ARROW].second = m_nodes.size();
    m_parts[MY_POSITION_POINT].first = batcher.InsertTriangleStrip(state, make_ref(&pointProvider), nullptr);
    ASSERT(m_parts[MY_POSITION_POINT].first.IsValid(), ());
    m_parts[MY_POSITION_ARROW].first = batcher.InsertTriangleStrip(state, make_ref(&arrowProvider), nullptr);
    ASSERT(m_parts[MY_POSITION_ARROW].first.IsValid(), ());
  }
}

void MyPosition::RenderPart(ref_ptr<dp::GpuProgramManager> mng,
                            dp::UniformValuesStorage const & uniforms,
                            MyPosition::EMyPositionPart part)
{
  TPart const & accuracy = m_parts[part];
  RenderNode & node = m_nodes[accuracy.second];

  ref_ptr<dp::GpuProgram> prg = mng->GetProgram(node.m_state.GetProgramIndex());
  prg->Bind();
  if (!node.m_isBuilded)
  {
    node.m_buffer->Build(prg);
    node.m_isBuilded = true;
  }

  dp::ApplyUniforms(uniforms, prg);
  dp::ApplyState(node.m_state, prg);

  node.m_buffer->RenderRange(accuracy.first);
}

}
