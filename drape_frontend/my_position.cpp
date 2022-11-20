#include "drape_frontend/my_position.hpp"
#include "drape_frontend/batcher_bucket.hpp"
#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "shaders/programs.hpp"

#include "drape/constants.hpp"
#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/render_bucket.hpp"


namespace df
{
namespace mp
{
df::ColorConstant const kMyPositionAccuracyColor = "MyPositionAccuracy";

struct MarkerVertex
{
  MarkerVertex() = default;
  MarkerVertex(glsl::vec2 const & normal, glsl::vec2 const & texCoord)
    : m_normal(normal)
    , m_texCoord(texCoord)
  {}

  glsl::vec2 m_normal;
  glsl::vec2 m_texCoord;
};

dp::BindingInfo GetMarkerBindingInfo()
{
  dp::BindingInfo info(2);
  dp::BindingDecl & normal = info.GetBindingDecl(0);
  normal.m_attributeName = "a_normal";
  normal.m_componentCount = 2;
  normal.m_componentType = gl_const::GLFloatType;
  normal.m_offset = 0;
  normal.m_stride = sizeof(MarkerVertex);

  dp::BindingDecl & texCoord = info.GetBindingDecl(1);
  texCoord.m_attributeName = "a_colorTexCoords";
  texCoord.m_componentCount = 2;
  texCoord.m_componentType = gl_const::GLFloatType;
  texCoord.m_offset = sizeof(glsl::vec2);
  texCoord.m_stride = sizeof(MarkerVertex);

  return info;
}
}  // namespace mp

MyPosition::MyPosition(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> mng)
  : m_position(m2::PointF::Zero())
  , m_azimuth(0.0f)
  , m_accuracy(0.0f)
  , m_showAzimuth(false)
  , m_isRoutingMode(false)
{
  m_parts.resize(4);
  CacheAccuracySector(context, mng);
  CachePointPosition(context, mng);
}

void MyPosition::InitArrow(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> mng)
{
  m_arrow3d = make_unique_dp<Arrow3d>(context);
  m_arrow3d->SetTexture(mng);
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
  CHECK(m_arrow3d != nullptr, ());
  m_arrow3d->SetPositionObsolete(obsolete);
}

void MyPosition::RenderAccuracy(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                ScreenBase const & screen, int zoomLevel, FrameValues const & frameValues)
{
  m2::PointD accuracyPoint(m_position.x + m_accuracy, m_position.y);
  auto const pixelAccuracy =
    static_cast<float>((screen.GtoP(accuracyPoint) - screen.GtoP(m2::PointD(m_position))).Length());

  gpu::ShapesProgramParams params;
  frameValues.SetTo(params);
  TileKey const key = GetTileKeyByPoint(m2::PointD(m_position), ClipTileZoomByMaxDataZoom(zoomLevel));
  math::Matrix<float, 4, 4> mv = key.GetTileBasedModelView(screen);
  params.m_modelView = glsl::make_mat4(mv.m_data);

  auto const pos = static_cast<m2::PointF>(
    MapShape::ConvertToLocal(m2::PointD(m_position), key.GetGlobalRect().Center(), kShapeCoordScalar));
  params.m_position = glsl::vec3(pos.x, pos.y, 0.0f);
  params.m_accuracy = pixelAccuracy;
  RenderPart(context, mng, params, MyPositionAccuracy);
}

void MyPosition::RenderMyPosition(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                  ScreenBase const & screen, int zoomLevel, FrameValues const & frameValues)
{
  if (m_showAzimuth)
  {
    CHECK(m_arrow3d != nullptr, ());
    m_arrow3d->SetPosition(m2::PointD(m_position));
    m_arrow3d->SetAzimuth(m_azimuth);
    m_arrow3d->Render(context, mng, screen, m_isRoutingMode);
  }
  else
  {
    gpu::ShapesProgramParams params;
    frameValues.SetTo(params);
    TileKey const key = GetTileKeyByPoint(m2::PointD(m_position), ClipTileZoomByMaxDataZoom(zoomLevel));
    math::Matrix<float, 4, 4> mv = key.GetTileBasedModelView(screen);
    params.m_modelView = glsl::make_mat4(mv.m_data);

    auto const pos = static_cast<m2::PointF>(
      MapShape::ConvertToLocal(m2::PointD(m_position), key.GetGlobalRect().Center(), kShapeCoordScalar));
    params.m_position = glsl::vec3(pos.x, pos.y, dp::depth::kMyPositionMarkDepth);
    params.m_azimut = -(m_azimuth + static_cast<float>(screen.GetAngle()));
    RenderPart(context, mng, params, MyPositionPoint);
  }
}

void MyPosition::CacheAccuracySector(ref_ptr<dp::GraphicsContext> context,
                                     ref_ptr<dp::TextureManager> mng)
{
  size_t constexpr kTriangleCount = 40;
  size_t constexpr kVertexCount = 3 * kTriangleCount;
  auto const etalonSector = static_cast<float>(2.0 * math::pi / kTriangleCount);

  dp::TextureManager::ColorRegion color;
  mng->GetColorRegion(df::GetColorConstant(mp::kMyPositionAccuracyColor), color);
  glsl::vec2 colorCoord = glsl::ToVec2(color.GetTexRect().Center());

  buffer_vector<mp::MarkerVertex, kTriangleCount> buffer;
  glsl::vec2 startNormal(0.0f, 1.0f);

  for (size_t i = 0; i < kTriangleCount + 1; ++i)
  {
    glsl::vec2 normal = glsl::rotate(startNormal, i * etalonSector);
    glsl::vec2 nextNormal = glsl::rotate(startNormal, (i + 1) * etalonSector);

    buffer.emplace_back(glsl::vec2(0.0f, 0.0f), colorCoord);
    buffer.emplace_back(normal, colorCoord);
    buffer.emplace_back(nextNormal, colorCoord);
  }

  auto state = CreateRenderState(gpu::Program::Accuracy, DepthLayer::OverlayLayer);
  state.SetDepthTestEnabled(false);
  state.SetColorTexture(color.GetTexture());

  {
    dp::Batcher batcher(kTriangleCount * dp::Batcher::IndexPerTriangle, kVertexCount);
    batcher.SetBatcherHash(static_cast<uint64_t>(BatcherBucket::Default));
    dp::SessionGuard guard(context, batcher,
      [this](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      drape_ptr<dp::RenderBucket> bucket = std::move(b);
      ASSERT(bucket->GetOverlayHandlesCount() == 0, ());

      m_nodes.emplace_back(state, bucket->MoveBuffer());
      m_parts[MyPositionAccuracy].second = m_nodes.size() - 1;
    });

    dp::AttributeProvider provider(1 /* stream count */, kVertexCount);
    provider.InitStream(0 /* stream index */, mp::GetMarkerBindingInfo(), make_ref(buffer.data()));

    m_parts[MyPositionAccuracy].first = batcher.InsertTriangleList(context, state,
                                                                   make_ref(&provider), nullptr);
    ASSERT(m_parts[MyPositionAccuracy].first.IsValid(), ());
  }
}

void MyPosition::CacheSymbol(ref_ptr<dp::GraphicsContext> context,
                             dp::TextureManager::SymbolRegion const & symbol,
                             dp::RenderState const & state, dp::Batcher & batcher,
                             EMyPositionPart part)
{
  m2::RectF const & texRect = symbol.GetTexRect();
  m2::PointF const halfSize = symbol.GetPixelSize() * 0.5f;

  mp::MarkerVertex data[4] =
  {
    { glsl::vec2(-halfSize.x,  halfSize.y), glsl::ToVec2(texRect.LeftTop()) },
    { glsl::vec2(-halfSize.x, -halfSize.y), glsl::ToVec2(texRect.LeftBottom()) },
    { glsl::vec2( halfSize.x,  halfSize.y), glsl::ToVec2(texRect.RightTop()) },
    { glsl::vec2( halfSize.x, -halfSize.y), glsl::ToVec2(texRect.RightBottom())}
  };

  dp::AttributeProvider provider(1 /* streamCount */, dp::Batcher::VertexPerQuad);
  provider.InitStream(0 /* streamIndex */, mp::GetMarkerBindingInfo(), make_ref(data));
  m_parts[part].first = batcher.InsertTriangleStrip(context, state, make_ref(&provider), nullptr);
  ASSERT(m_parts[part].first.IsValid(), ());
}

void MyPosition::CachePointPosition(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> mng)
{
  int const kSymbolsCount = 1;
  dp::TextureManager::SymbolRegion pointSymbol;
  mng->GetSymbolRegion("current-position", pointSymbol);

  auto state = CreateRenderState(gpu::Program::MyPosition, DepthLayer::OverlayLayer);
  state.SetDepthTestEnabled(false);
  state.SetColorTexture(pointSymbol.GetTexture());
  state.SetTextureIndex(pointSymbol.GetTextureIndex());

  dp::TextureManager::SymbolRegion * symbols[kSymbolsCount] = { &pointSymbol };
  EMyPositionPart partIndices[kSymbolsCount] = { MyPositionPoint };
  {
    dp::Batcher batcher(kSymbolsCount * dp::Batcher::IndexPerQuad, kSymbolsCount * dp::Batcher::VertexPerQuad);
    batcher.SetBatcherHash(static_cast<uint64_t>(BatcherBucket::Default));
    dp::SessionGuard guard(context, batcher,
      [this](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      drape_ptr<dp::RenderBucket> bucket = std::move(b);
      ASSERT(bucket->GetOverlayHandlesCount() == 0, ());

      m_nodes.emplace_back(state, bucket->MoveBuffer());
    });

    auto const partIndex = m_nodes.size();
    for (int i = 0; i < kSymbolsCount; i++)
    {
      m_parts[partIndices[i]].second = partIndex;
      CacheSymbol(context, *symbols[i], state, batcher, partIndices[i]);
    }
  }
}

void MyPosition::RenderPart(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                            gpu::ShapesProgramParams const & params, EMyPositionPart part)
{
  TPart const & p = m_parts[part];
  m_nodes[p.second].Render(context, mng, params, p.first);
}
}  // namespace df
