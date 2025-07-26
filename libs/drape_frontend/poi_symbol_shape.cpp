#include "drape_frontend/poi_symbol_shape.hpp"

#include "drape_frontend/color_constants.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "base/buffer_vector.hpp"

#include <utility>

namespace
{
using SV = gpu::SolidTexturingVertex;
using MV = gpu::MaskedTexturingVertex;

glsl::vec2 ShiftNormal(glsl::vec2 const & n, df::PoiSymbolViewParams const & params, m2::PointF const & pixelSize)
{
  glsl::vec2 result = n + glsl::vec2(params.m_offset.x, params.m_offset.y);
  m2::PointF const halfPixelSize = pixelSize * 0.5f;

  if (params.m_anchor & dp::Top)
    result.y += halfPixelSize.y;
  else if (params.m_anchor & dp::Bottom)
    result.y -= halfPixelSize.y;

  if (params.m_anchor & dp::Left)
    result.x += halfPixelSize.x;
  else if (params.m_anchor & dp::Right)
    result.x -= halfPixelSize.x;

  return result;
}

template <typename TCreateOverlayHandle>
void SolidBatch(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                TCreateOverlayHandle && createOverlayHandle, glsl::vec4 const & position,
                df::PoiSymbolViewParams const & params, dp::TextureManager::SymbolRegion const & symbolRegion,
                dp::TextureManager::ColorRegion const & colorRegion)
{
  UNUSED_VALUE(colorRegion);

  m2::PointF const symbolRegionSize = symbolRegion.GetPixelSize();
  m2::PointF pixelSize = symbolRegionSize;
  if (pixelSize.x < params.m_pixelWidth)
    pixelSize.x = params.m_pixelWidth;
  m2::PointF const halfSize = pixelSize * 0.5f;
  m2::RectF const & texRect = symbolRegion.GetTexRect();

  buffer_vector<SV, 8> vertexes;
  vertexes.emplace_back(position, ShiftNormal(glsl::vec2(-halfSize.x, halfSize.y), params, pixelSize),
                        glsl::vec2(texRect.minX(), texRect.maxY()));
  vertexes.emplace_back(position, ShiftNormal(glsl::vec2(-halfSize.x, -halfSize.y), params, pixelSize),
                        glsl::vec2(texRect.minX(), texRect.minY()));
  if (symbolRegionSize.x < params.m_pixelWidth)
  {
    float const stretchHalfWidth = (params.m_pixelWidth - symbolRegionSize.x) * 0.5f;
    float const midTexU = (texRect.minX() + texRect.maxX()) * 0.5f;
    vertexes.emplace_back(position, ShiftNormal(glsl::vec2(-stretchHalfWidth, halfSize.y), params, pixelSize),
                          glsl::vec2(midTexU, texRect.maxY()));
    vertexes.emplace_back(position, ShiftNormal(glsl::vec2(-stretchHalfWidth, -halfSize.y), params, pixelSize),
                          glsl::vec2(midTexU, texRect.minY()));
    vertexes.emplace_back(position, ShiftNormal(glsl::vec2(stretchHalfWidth, halfSize.y), params, pixelSize),
                          glsl::vec2(midTexU, texRect.maxY()));
    vertexes.emplace_back(position, ShiftNormal(glsl::vec2(stretchHalfWidth, -halfSize.y), params, pixelSize),
                          glsl::vec2(midTexU, texRect.minY()));
  }
  vertexes.emplace_back(position, ShiftNormal(glsl::vec2(halfSize.x, halfSize.y), params, pixelSize),
                        glsl::vec2(texRect.maxX(), texRect.maxY()));
  vertexes.emplace_back(position, ShiftNormal(glsl::vec2(halfSize.x, -halfSize.y), params, pixelSize),
                        glsl::vec2(texRect.maxX(), texRect.minY()));

  auto state = df::CreateRenderState(gpu::Program::Texturing, params.m_depthLayer);
  state.SetProgram3d(gpu::Program::TexturingBillboard);
  state.SetDepthTestEnabled(params.m_depthTestEnabled);
  state.SetColorTexture(symbolRegion.GetTexture());
  state.SetTextureFilter(dp::TextureFilter::Nearest);
  state.SetTextureIndex(symbolRegion.GetTextureIndex());

  dp::AttributeProvider provider(1 /* streamCount */, static_cast<uint32_t>(vertexes.size()));
  provider.InitStream(0 /* streamIndex */, SV::GetBindingInfo(), make_ref(vertexes.data()));
  batcher->InsertTriangleStrip(context, state, make_ref(&provider), createOverlayHandle(vertexes));
}

template <typename TCreateOverlayHandle>
void MaskedBatch(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                 TCreateOverlayHandle && createOverlayHandle, glsl::vec4 const & position,
                 df::PoiSymbolViewParams const & params, dp::TextureManager::SymbolRegion const & symbolRegion,
                 dp::TextureManager::ColorRegion const & colorRegion)
{
  m2::PointF const symbolRegionSize = symbolRegion.GetPixelSize();
  m2::PointF pixelSize = symbolRegionSize;
  if (pixelSize.x < params.m_pixelWidth)
    pixelSize.x = params.m_pixelWidth;
  m2::PointF const halfSize = pixelSize * 0.5f;
  m2::RectF const & texRect = symbolRegion.GetTexRect();
  glsl::vec2 const maskColorCoords = glsl::ToVec2(colorRegion.GetTexRect().Center());

  buffer_vector<MV, 8> vertexes;
  vertexes.emplace_back(position, ShiftNormal(glsl::vec2(-halfSize.x, halfSize.y), params, pixelSize),
                        glsl::vec2(texRect.minX(), texRect.maxY()), maskColorCoords);
  vertexes.emplace_back(position, ShiftNormal(glsl::vec2(-halfSize.x, -halfSize.y), params, pixelSize),
                        glsl::vec2(texRect.minX(), texRect.minY()), maskColorCoords);
  if (symbolRegionSize.x < params.m_pixelWidth)
  {
    float const stretchHalfWidth = (params.m_pixelWidth - symbolRegionSize.x) * 0.5f;
    float const midTexU = (texRect.minX() + texRect.maxX()) * 0.5f;
    vertexes.emplace_back(position, ShiftNormal(glsl::vec2(-stretchHalfWidth, halfSize.y), params, pixelSize),
                          glsl::vec2(midTexU, texRect.maxY()), maskColorCoords);
    vertexes.emplace_back(position, ShiftNormal(glsl::vec2(-stretchHalfWidth, -halfSize.y), params, pixelSize),
                          glsl::vec2(midTexU, texRect.minY()), maskColorCoords);
    vertexes.emplace_back(position, ShiftNormal(glsl::vec2(stretchHalfWidth, halfSize.y), params, pixelSize),
                          glsl::vec2(midTexU, texRect.maxY()), maskColorCoords);
    vertexes.emplace_back(position, ShiftNormal(glsl::vec2(stretchHalfWidth, -halfSize.y), params, pixelSize),
                          glsl::vec2(midTexU, texRect.minY()), maskColorCoords);
  }
  vertexes.emplace_back(position, ShiftNormal(glsl::vec2(halfSize.x, halfSize.y), params, pixelSize),
                        glsl::vec2(texRect.maxX(), texRect.maxY()), maskColorCoords);
  vertexes.emplace_back(position, ShiftNormal(glsl::vec2(halfSize.x, -halfSize.y), params, pixelSize),
                        glsl::vec2(texRect.maxX(), texRect.minY()), maskColorCoords);

  auto state = df::CreateRenderState(gpu::Program::MaskedTexturing, params.m_depthLayer);
  state.SetProgram3d(gpu::Program::MaskedTexturingBillboard);
  state.SetDepthTestEnabled(params.m_depthTestEnabled);
  state.SetColorTexture(symbolRegion.GetTexture());
  state.SetMaskTexture(colorRegion.GetTexture());  // Here mask is a color.
  state.SetTextureFilter(dp::TextureFilter::Nearest);
  state.SetTextureIndex(symbolRegion.GetTextureIndex());

  dp::AttributeProvider provider(1 /* streamCount */, static_cast<uint32_t>(vertexes.size()));
  provider.InitStream(0 /* streamIndex */, MV::GetBindingInfo(), make_ref(vertexes.data()));
  batcher->InsertTriangleStrip(context, state, make_ref(&provider), createOverlayHandle(vertexes));
}
}  // namespace

namespace df
{
PoiSymbolShape::PoiSymbolShape(m2::PointD const & mercatorPt, PoiSymbolViewParams const & params,
                               TileKey const & tileKey, uint32_t textIndex)
  : m_pt(mercatorPt)
  , m_params(params)
  , m_tileCoords(tileKey.GetTileCoords())
  , m_textIndex(textIndex)
{}

void PoiSymbolShape::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                          ref_ptr<dp::TextureManager> textures) const
{
  dp::TextureManager::SymbolRegion region;
  textures->GetSymbolRegion(m_params.m_symbolName, region);

  glsl::vec2 const pt = glsl::ToVec2(ConvertToLocal(m_pt, m_params.m_tileCenter, kShapeCoordScalar));
  // TODO: if m_params.m_depthTestEnabled == false then passing a real
  // m_params.m_depth value to OGL doesn't make sense, but could lead to
  // elements out of [dp::kMinDepth, dp::kMaxDepth] depth range being
  // not rendered at all. E.g. depth values for overlays are derived from priorities
  // hence it leads to unnecessary restriction of overlays priorities range.
  // The same is true for TextShape, ColoredSymbolShape etc.
  glsl::vec4 const position = glsl::vec4(pt, m_params.m_depth, -m_params.m_posZ);

  auto const createOverlayHandle = [this](auto const & vertexes) -> drape_ptr<dp::OverlayHandle>
  {
    m2::RectD rect;
    for (auto const & vertex : vertexes)
      rect.Add(glsl::FromVec2(vertex.m_normal));
    return CreateOverlayHandle(rect);
  };

  dp::TextureManager::ColorRegion maskColorRegion;
  if (m_params.m_maskColor.empty())
  {
    SolidBatch(context, batcher, createOverlayHandle, position, m_params, region, maskColorRegion);
  }
  else
  {
    textures->GetColorRegion(df::GetColorConstant(m_params.m_maskColor), maskColorRegion);
    MaskedBatch(context, batcher, createOverlayHandle, position, m_params, region, maskColorRegion);
  }
}

drape_ptr<dp::OverlayHandle> PoiSymbolShape::CreateOverlayHandle(m2::RectD const & pixelRect) const
{
  dp::OverlayID overlayId(m_params.m_featureId, m_params.m_markId, m_tileCoords, m_textIndex);
  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<dp::SquareHandle>(
      overlayId, m_params.m_anchor, m_pt, pixelRect.RightTop() - pixelRect.LeftBottom(), m2::PointD(m_params.m_offset),
      GetOverlayPriority(), true /* isBound */, m_params.m_minVisibleScale, true /* isBillboard */);
  handle->SetPivotZ(m_params.m_posZ);
  handle->SetExtendingSize(m_params.m_extendingSize);
  if (m_params.m_specialDisplacement == SpecialDisplacement::UserMark ||
      m_params.m_specialDisplacement == SpecialDisplacement::SpecialModeUserMark)
  {
    handle->SetSpecialLayerOverlay(true);
  }
  handle->SetOverlayRank(m_params.m_startOverlayRank);
  return handle;
}

uint64_t PoiSymbolShape::GetOverlayPriority() const
{
  // Set up maximum priority for some POI.
  if (m_params.m_prioritized)
    return dp::kPriorityMaskAll;

  if (m_params.m_specialDisplacement == SpecialDisplacement::SpecialModeUserMark)
    return dp::CalculateSpecialModeUserMarkPriority(m_params.m_specialPriority);

  if (m_params.m_specialDisplacement == SpecialDisplacement::UserMark)
    return dp::CalculateUserMarkPriority(m_params.m_minVisibleScale, m_params.m_specialPriority);

  return dp::CalculateOverlayPriority(m_params.m_rank, m_params.m_depth);
}
}  // namespace df
