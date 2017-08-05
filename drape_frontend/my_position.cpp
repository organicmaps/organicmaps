#include "drape_frontend/my_position.hpp"
#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shader_def.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "drape/constants.hpp"
#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/render_bucket.hpp"

#include "indexer/map_style_reader.hpp"

namespace df
{
namespace
{
df::ColorConstant const kMyPositionAccuracyColor = "MyPositionAccuracy";

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
} //  namespace

MyPosition::MyPosition(ref_ptr<dp::TextureManager> mng)
  : m_position(m2::PointF::Zero())
  , m_azimuth(0.0f)
  , m_accuracy(0.0f)
  , m_showAzimuth(false)
  , m_isRoutingMode(false)
  , m_obsoletePosition(false)
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

void MyPosition::SetPositionObsolete(bool obsolete)
{
  m_obsoletePosition = obsolete;
  m_arrow3d.SetPositionObsolete(obsolete);
}

void MyPosition::RenderAccuracy(ScreenBase const & screen, int zoomLevel,
                                ref_ptr<dp::GpuProgramManager> mng,
                                dp::UniformValuesStorage const & commonUniforms)
{
  dp::UniformValuesStorage uniforms = commonUniforms;
  m2::PointD accuracyPoint(m_position.x + m_accuracy, m_position.y);
  float pixelAccuracy = (screen.GtoP(accuracyPoint) - screen.GtoP(m_position)).Length();

  TileKey const key = GetTileKeyByPoint(m_position, ClipTileZoomByMaxDataZoom(zoomLevel));
  math::Matrix<float, 4, 4> mv = key.GetTileBasedModelView(screen);
  uniforms.SetMatrix4x4Value("modelView", mv.m_data);

  m2::PointD const pos = MapShape::ConvertToLocal(m_position, key.GetGlobalRect().Center(), kShapeCoordScalar);
  uniforms.SetFloatValue("u_position", pos.x, pos.y, 0.0f);
  uniforms.SetFloatValue("u_accuracy", pixelAccuracy);
  uniforms.SetFloatValue("u_opacity", 1.0f);
  RenderPart(mng, uniforms, MY_POSITION_ACCURACY);
}

void MyPosition::RenderMyPosition(ScreenBase const & screen, int zoomLevel,
                                  ref_ptr<dp::GpuProgramManager> mng,
                                  dp::UniformValuesStorage const & commonUniforms)
{
  if (m_showAzimuth)
  {
    m_arrow3d.SetPosition(m_position);
    m_arrow3d.SetAzimuth(m_azimuth);
    m_arrow3d.Render(screen, mng, m_isRoutingMode);
  }
  else
  {
    dp::UniformValuesStorage uniforms = commonUniforms;
    TileKey const key = GetTileKeyByPoint(m_position, ClipTileZoomByMaxDataZoom(zoomLevel));
    math::Matrix<float, 4, 4> mv = key.GetTileBasedModelView(screen);
    uniforms.SetMatrix4x4Value("modelView", mv.m_data);

    m2::PointD const pos = MapShape::ConvertToLocal(m_position, key.GetGlobalRect().Center(), kShapeCoordScalar);
    uniforms.SetFloatValue("u_position", pos.x, pos.y, dp::depth::kMyPositionMarkDepth);
    uniforms.SetFloatValue("u_azimut", -(m_azimuth + screen.GetAngle()));
    uniforms.SetFloatValue("u_opacity", 1.0);
    RenderPart(mng, uniforms, MY_POSITION_POINT);
  }
}

void MyPosition::CacheAccuracySector(ref_ptr<dp::TextureManager> mng)
{
  int const TriangleCount = 40;
  int const VertexCount = 3 * TriangleCount;
  float const etalonSector = math::twicePi / static_cast<double>(TriangleCount);

  dp::TextureManager::ColorRegion color;
  mng->GetColorRegion(df::GetColorConstant(df::kMyPositionAccuracyColor), color);
  glsl::vec2 colorCoord = glsl::ToVec2(color.GetTexRect().Center());

  buffer_vector<Vertex, TriangleCount> buffer;
  glsl::vec2 startNormal(0.0f, 1.0f);

  for (size_t i = 0; i < TriangleCount + 1; ++i)
  {
    glsl::vec2 normal = glsl::rotate(startNormal, i * etalonSector);
    glsl::vec2 nextNormal = glsl::rotate(startNormal, (i + 1) * etalonSector);

    buffer.emplace_back(startNormal, colorCoord);
    buffer.emplace_back(normal, colorCoord);
    buffer.emplace_back(nextNormal, colorCoord);
  }

  auto state = CreateGLState(gpu::ACCURACY_PROGRAM, RenderState::OverlayLayer);
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

void MyPosition::CacheSymbol(dp::TextureManager::SymbolRegion const & symbol,
                             dp::GLState const & state, dp::Batcher & batcher,
                             EMyPositionPart part)
{
  m2::RectF const & texRect = symbol.GetTexRect();
  m2::PointF const halfSize = symbol.GetPixelSize() * 0.5f;

  Vertex data[4] =
  {
    { glsl::vec2(-halfSize.x,  halfSize.y), glsl::ToVec2(texRect.LeftTop()) },
    { glsl::vec2(-halfSize.x, -halfSize.y), glsl::ToVec2(texRect.LeftBottom()) },
    { glsl::vec2( halfSize.x,  halfSize.y), glsl::ToVec2(texRect.RightTop()) },
    { glsl::vec2( halfSize.x, -halfSize.y), glsl::ToVec2(texRect.RightBottom())}
  };

  dp::AttributeProvider provider(1 /* streamCount */, dp::Batcher::VertexPerQuad);
  provider.InitStream(0 /* streamIndex */, GetBindingInfo(), make_ref(data));
  m_parts[part].first = batcher.InsertTriangleStrip(state, make_ref(&provider), nullptr);
  ASSERT(m_parts[part].first.IsValid(), ());
}

void MyPosition::CachePointPosition(ref_ptr<dp::TextureManager> mng)
{
  int const kSymbolsCount = 1;
  dp::TextureManager::SymbolRegion pointSymbol;
  mng->GetSymbolRegion("current-position", pointSymbol);

  m_arrow3d.SetTexture(mng);

  auto state = CreateGLState(gpu::MY_POSITION_PROGRAM, RenderState::OverlayLayer);
  state.SetColorTexture(pointSymbol.GetTexture());

  dp::TextureManager::SymbolRegion * symbols[kSymbolsCount] = { &pointSymbol };
  EMyPositionPart partIndices[kSymbolsCount] = { MY_POSITION_POINT };
  {
    dp::Batcher batcher(kSymbolsCount * dp::Batcher::IndexPerQuad, kSymbolsCount * dp::Batcher::VertexPerQuad);
    dp::SessionGuard guard(batcher, [this](dp::GLState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      drape_ptr<dp::RenderBucket> bucket = move(b);
      ASSERT(bucket->GetOverlayHandlesCount() == 0, ());

      m_nodes.emplace_back(state, bucket->MoveBuffer());
    });

    int const partIndex = static_cast<int>(m_nodes.size());
    for (int i = 0; i < kSymbolsCount; i++)
    {
      m_parts[partIndices[i]].second = partIndex;
      CacheSymbol(*symbols[i], state, batcher, partIndices[i]);
    }
  }
}

void MyPosition::RenderPart(ref_ptr<dp::GpuProgramManager> mng,
                            dp::UniformValuesStorage const & uniforms,
                            MyPosition::EMyPositionPart part)
{
  TPart const & p = m_parts[part];
  m_nodes[p.second].Render(mng, uniforms, p.first);
}

} // namespace df
