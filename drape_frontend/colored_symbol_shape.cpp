#include "drape_frontend/colored_symbol_shape.hpp"

#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

namespace df
{
namespace
{
glsl::vec2 ShiftNormal(glsl::vec2 const & n, ColoredSymbolViewParams const & params)
{
  glsl::vec2 result = n + glsl::vec2(params.m_offset.x, params.m_offset.y);
  m2::PointF halfPixelSize;
  if (params.m_shape == ColoredSymbolViewParams::Shape::Circle)
    halfPixelSize = m2::PointF(params.m_radiusInPixels, params.m_radiusInPixels);
  else
    halfPixelSize = m2::PointF(0.5f * params.m_sizeInPixels.x, 0.5f * params.m_sizeInPixels.y);

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
}  // namespace

class DynamicSquareHandle : public dp::SquareHandle
{
  using TBase = dp::SquareHandle;

public:
  DynamicSquareHandle(dp::OverlayID const & id, dp::Anchor anchor, m2::PointD const & gbPivot,
                      std::vector<m2::PointF> const & pxSizes, m2::PointD const & pxOffset,
                      uint64_t priority, bool isBound, std::string const & debugStr,
                      int minVisibleScale, bool isBillboard)
    : TBase(id, anchor, gbPivot, m2::PointD::Zero(), pxOffset, priority, isBound, debugStr, minVisibleScale,
            isBillboard)
    , m_pxSizes(pxSizes)
  {
    ASSERT_GREATER(pxSizes.size(), 0, ());
  }

  bool Update(ScreenBase const & screen) override
  {
    double zoom = 0.0;
    int index = 0;
    float lerpCoef = 0.0f;
    ExtractZoomFactors(screen, zoom, index, lerpCoef);
    auto const size = InterpolateByZoomLevels(index, lerpCoef, m_pxSizes);
    m_pxHalfSize.x = size.x * 0.5;
    m_pxHalfSize.y = size.y * 0.5;
    return true;
  }

private:
  std::vector<m2::PointF> m_pxSizes;
};

ColoredSymbolShape::ColoredSymbolShape(m2::PointD const & mercatorPt, ColoredSymbolViewParams const & params,
                                       TileKey const & tileKey, uint32_t textIndex, bool needOverlay)
  : m_point(mercatorPt)
  , m_params(params)
  , m_tileCoords(tileKey.GetTileCoords())
  , m_textIndex(textIndex)
  , m_needOverlay(needOverlay)
{}

ColoredSymbolShape::ColoredSymbolShape(m2::PointD const & mercatorPt,
                                       ColoredSymbolViewParams const & params,
                                       TileKey const & tileKey, uint32_t textIndex,
                                       std::vector<m2::PointF> const & overlaySizes)
  : m_point(mercatorPt)
  , m_params(params)
  , m_tileCoords(tileKey.GetTileCoords())
  , m_textIndex(textIndex)
  , m_needOverlay(true)
  , m_overlaySizes(overlaySizes)
{}

void ColoredSymbolShape::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                              ref_ptr<dp::TextureManager> textures) const
{
  dp::TextureManager::ColorRegion colorRegion;
  textures->GetColorRegion(m_params.m_color, colorRegion);
  m2::PointF const & colorUv = colorRegion.GetTexRect().Center();

  dp::TextureManager::ColorRegion outlineColorRegion;
  textures->GetColorRegion(m_params.m_outlineColor, outlineColorRegion);
  m2::PointF const & outlineUv = outlineColorRegion.GetTexRect().Center();

  using V = gpu::ColoredSymbolVertex;
  V::TTexCoord const uv(colorUv.x, colorUv.y, 0.0f, 0.0f);
  V::TTexCoord const uvOutline(outlineUv.x, outlineUv.y, 0.0f, 0.0f);

  glsl::vec2 const pt = glsl::ToVec2(ConvertToLocal(m_point, m_params.m_tileCenter,
                                                    kShapeCoordScalar));
  glsl::vec3 const position = glsl::vec3(pt, m_params.m_depth);

  buffer_vector<V, 48> buffer;

  auto norm = [this](float x, float y)
  {
    return ShiftNormal(glsl::vec2(x, y), m_params);
  };

  m2::PointU pixelSize;
  if (m_params.m_shape == ColoredSymbolViewParams::Shape::Circle)
  {
    pixelSize = m2::PointU(2 * static_cast<uint32_t>(m_params.m_radiusInPixels),
                           2 * static_cast<uint32_t>(m_params.m_radiusInPixels));
    // Here we use an equilateral triangle to render circle (incircle of a triangle).
    static float const kSqrt3 = static_cast<float>(sqrt(3.0f));
    float r = m_params.m_radiusInPixels - m_params.m_outlineWidth;

    V::TTexCoord uv2(uv.x, uv.y, norm(0.0, 0.0));
    buffer.push_back(V(position, V::TNormal(-r * kSqrt3, -r, r, 1.0f), uv2));
    buffer.push_back(V(position, V::TNormal(r * kSqrt3, -r, r, 1.0f), uv2));
    buffer.push_back(V(position, V::TNormal(0.0f, 2.0f * r, r, 1.0f), uv2));

    if (m_params.m_outlineWidth >= 1e-5)
    {
      r = m_params.m_radiusInPixels;
      V::TTexCoord uvOutline2(uvOutline.x, uvOutline.y, norm(0.0, 0.0));
      buffer.push_back(V(position, V::TNormal(-r * kSqrt3, -r, r, 1.0f), uvOutline2));
      buffer.push_back(V(position, V::TNormal(r * kSqrt3, -r, r, 1.0f), uvOutline2));
      buffer.push_back(V(position, V::TNormal(0.0f, 2.0f * r, r, 1.0f), uvOutline2));
    }
  }
  else if (m_params.m_shape == ColoredSymbolViewParams::Shape::Rectangle)
  {
    pixelSize = m2::PointU(static_cast<uint32_t>(m_params.m_sizeInPixels.x),
                           static_cast<uint32_t>(m_params.m_sizeInPixels.y));
    float const halfWidth = 0.5f * m_params.m_sizeInPixels.x;
    float const halfHeight = 0.5f * m_params.m_sizeInPixels.y;
    float const v = halfWidth * halfWidth + halfHeight * halfHeight;
    float const halfWidthInside = halfWidth - m_params.m_outlineWidth;
    float const halfHeightInside = halfHeight - m_params.m_outlineWidth;

    buffer.push_back(V(position, V::TNormal(norm(-halfWidthInside, halfHeightInside), v, 0.0f), uv));
    buffer.push_back(V(position, V::TNormal(norm(halfWidthInside, -halfHeightInside), v, 0.0f), uv));
    buffer.push_back(V(position, V::TNormal(norm(halfWidthInside, halfHeightInside), v, 0.0f), uv));
    buffer.push_back(V(position, V::TNormal(norm(-halfWidthInside, halfHeightInside), v, 0.0f), uv));
    buffer.push_back(V(position, V::TNormal(norm(-halfWidthInside, -halfHeightInside), v, 0.0f), uv));
    buffer.push_back(V(position, V::TNormal(norm(halfWidthInside, -halfHeightInside), v, 0.0f), uv));

    if (m_params.m_outlineWidth >= 1e-5)
    {
      buffer.push_back(V(position, V::TNormal(norm(-halfWidth, halfHeight), v, 0.0f), uvOutline));
      buffer.push_back(V(position, V::TNormal(norm(halfWidth, -halfHeight), v, 0.0f), uvOutline));
      buffer.push_back(V(position, V::TNormal(norm(halfWidth, halfHeight), v, 0.0f), uvOutline));
      buffer.push_back(V(position, V::TNormal(norm(-halfWidth, halfHeight), v, 0.0f), uvOutline));
      buffer.push_back(V(position, V::TNormal(norm(-halfWidth, -halfHeight), v, 0.0f), uvOutline));
      buffer.push_back(V(position, V::TNormal(norm(halfWidth, -halfHeight), v, 0.0f), uvOutline));
    }
  }
  else if (m_params.m_shape == ColoredSymbolViewParams::Shape::RoundedRectangle)
  {
    pixelSize = m2::PointU(static_cast<uint32_t>(m_params.m_sizeInPixels.x),
                           static_cast<uint32_t>(m_params.m_sizeInPixels.y));
    float const halfWidth = 0.5f * m_params.m_sizeInPixels.x;
    float const halfHeight = 0.5f * m_params.m_sizeInPixels.y;
    float const halfWidthBody = halfWidth - m_params.m_radiusInPixels;
    float const halfHeightBody = halfHeight - m_params.m_radiusInPixels;
    float const v = halfWidth * halfWidth + halfHeight * halfHeight;
    float const halfWidthInside = halfWidth - m_params.m_outlineWidth;
    float const halfHeightInside = halfHeight - m_params.m_outlineWidth;

    if (halfWidthBody > 0.0f && halfHeightInside > 0.0f)
    {
      buffer.push_back(V(position, V::TNormal(norm(-halfWidthBody, halfHeightInside), v, 0.0f), uv));
      buffer.push_back(V(position, V::TNormal(norm(halfWidthBody, -halfHeightInside), v, 0.0f), uv));
      buffer.push_back(V(position, V::TNormal(norm(halfWidthBody, halfHeightInside), v, 0.0f), uv));
      buffer.push_back(V(position, V::TNormal(norm(-halfWidthBody, halfHeightInside), v, 0.0f), uv));
      buffer.push_back(V(position, V::TNormal(norm(-halfWidthBody, -halfHeightInside), v, 0.0f), uv));
      buffer.push_back(V(position, V::TNormal(norm(halfWidthBody, -halfHeightInside), v, 0.0f), uv));
    }

    if (halfWidthInside > 0.0f && halfHeightBody > 0.0f)
    {
      buffer.push_back(V(position, V::TNormal(norm(-halfWidthInside, halfHeightBody), v, 0.0f), uv));
      buffer.push_back(V(position, V::TNormal(norm(halfWidthInside, -halfHeightBody), v, 0.0f), uv));
      buffer.push_back(V(position, V::TNormal(norm(halfWidthInside, halfHeightBody), v, 0.0f), uv));
      buffer.push_back(V(position, V::TNormal(norm(-halfWidthInside, halfHeightBody), v, 0.0f), uv));
      buffer.push_back(V(position, V::TNormal(norm(-halfWidthInside, -halfHeightBody), v, 0.0f), uv));
      buffer.push_back(V(position, V::TNormal(norm(halfWidthInside, -halfHeightBody), v, 0.0f), uv));
    }

    // Here we use an right triangle to render a quarter of circle.
    static float const kSqrt2 = static_cast<float>(sqrt(2.0f));
    float r = m_params.m_radiusInPixels - m_params.m_outlineWidth;
    V::TTexCoord uv2(uv.x, uv.y, norm(-halfWidthBody, halfHeightBody));
    buffer.push_back(V(position, V::TNormal(0.0, 0.0, r, 0.0f), uv2));
    buffer.push_back(V(position, V::TNormal(0.0, r * kSqrt2, r, 0.0f), uv2));
    buffer.push_back(V(position, V::TNormal(-r * kSqrt2, 0.0, r, 0.0f), uv2));

    uv2 = V::TTexCoord(uv.x, uv.y, norm(halfWidthBody, halfHeightBody));
    buffer.push_back(V(position, V::TNormal(0.0, 0.0, r, 0.0f), uv2));
    buffer.push_back(V(position, V::TNormal(r * kSqrt2, 0.0, r, 0.0f), uv2));
    buffer.push_back(V(position, V::TNormal(0.0, r * kSqrt2, r, 0.0f), uv2));

    uv2 = V::TTexCoord(uv.x, uv.y, norm(halfWidthBody, -halfHeightBody));
    buffer.push_back(V(position, V::TNormal(0.0, 0.0, r, 0.0f), uv2));
    buffer.push_back(V(position, V::TNormal(0.0, -r * kSqrt2, r, 0.0f), uv2));
    buffer.push_back(V(position, V::TNormal(r * kSqrt2, 0.0, r, 0.0f), uv2));

    uv2 = V::TTexCoord(uv.x, uv.y, norm(-halfWidthBody, -halfHeightBody));
    buffer.push_back(V(position, V::TNormal(0.0, 0.0, r, 0.0f), uv2));
    buffer.push_back(V(position, V::TNormal(-r * kSqrt2, 0.0, r, 0.0f), uv2));
    buffer.push_back(V(position, V::TNormal(0.0, -r * kSqrt2, r, 0.0f), uv2));

    if (m_params.m_outlineWidth >= 1e-5)
    {
      if (halfWidthBody > 0.0f && halfHeight > 0.0f)
      {
        buffer.push_back(V(position, V::TNormal(norm(-halfWidthBody, halfHeight), v, 0.0f), uvOutline));
        buffer.push_back(V(position, V::TNormal(norm(halfWidthBody, -halfHeight), v, 0.0f), uvOutline));
        buffer.push_back(V(position, V::TNormal(norm(halfWidthBody, halfHeight), v, 0.0f), uvOutline));
        buffer.push_back(V(position, V::TNormal(norm(-halfWidthBody, halfHeight), v, 0.0f), uvOutline));
        buffer.push_back(V(position, V::TNormal(norm(-halfWidthBody, -halfHeight), v, 0.0f), uvOutline));
        buffer.push_back(V(position, V::TNormal(norm(halfWidthBody, -halfHeight), v, 0.0f), uvOutline));
      }

      if (halfWidth > 0.0f && halfHeightBody > 0.0f)
      {
        buffer.push_back(V(position, V::TNormal(norm(-halfWidth, halfHeightBody), v, 0.0f), uvOutline));
        buffer.push_back(V(position, V::TNormal(norm(halfWidth, -halfHeightBody), v, 0.0f), uvOutline));
        buffer.push_back(V(position, V::TNormal(norm(halfWidth, halfHeightBody), v, 0.0f), uvOutline));
        buffer.push_back(V(position, V::TNormal(norm(-halfWidth, halfHeightBody), v, 0.0f), uvOutline));
        buffer.push_back(V(position, V::TNormal(norm(-halfWidth, -halfHeightBody), v, 0.0f), uvOutline));
        buffer.push_back(V(position, V::TNormal(norm(halfWidth, -halfHeightBody), v, 0.0f), uvOutline));
      }

      r = m_params.m_radiusInPixels;
      V::TTexCoord const uvOutline2(outlineUv.x, outlineUv.y, norm(-halfWidthBody, halfHeightBody));
      buffer.push_back(V(position, V::TNormal(0.0, 0.0, r, 0.0f), uvOutline2));
      buffer.push_back(V(position, V::TNormal(0.0, r * kSqrt2, r, 0.0f), uvOutline2));
      buffer.push_back(V(position, V::TNormal(-r * kSqrt2, 0.0, r, 0.0f), uvOutline2));

      uv2 = V::TTexCoord(outlineUv.x, outlineUv.y, norm(halfWidthBody, halfHeightBody));
      buffer.push_back(V(position, V::TNormal(0.0, 0.0, r, 0.0f), uv2));
      buffer.push_back(V(position, V::TNormal(r * kSqrt2, 0.0, r, 0.0f), uv2));
      buffer.push_back(V(position, V::TNormal(0.0, r * kSqrt2, r, 0.0f), uv2));

      uv2 = V::TTexCoord(outlineUv.x, outlineUv.y, norm(halfWidthBody, -halfHeightBody));
      buffer.push_back(V(position, V::TNormal(0.0, 0.0, r, 0.0f), uv2));
      buffer.push_back(V(position, V::TNormal(0.0, -r * kSqrt2, r, 0.0f), uv2));
      buffer.push_back(V(position, V::TNormal(r * kSqrt2, 0.0, r, 0.0f), uv2));

      uv2 = V::TTexCoord(outlineUv.x, outlineUv.y, norm(-halfWidthBody, -halfHeightBody));
      buffer.push_back(V(position, V::TNormal(0.0, 0.0, r, 0.0f), uv2));
      buffer.push_back(V(position, V::TNormal(-r * kSqrt2, 0.0, r, 0.0f), uv2));
      buffer.push_back(V(position, V::TNormal(0.0, -r * kSqrt2, r, 0.0f), uv2));
    }
  }

  if (buffer.empty())
    return;

  auto const overlayId = dp::OverlayID(m_params.m_featureID, m_tileCoords, m_textIndex);
  std::string const debugName = strings::to_string(m_params.m_featureID.m_index) + "-" +
                                strings::to_string(m_textIndex);

  drape_ptr<dp::OverlayHandle> handle;
  if (m_needOverlay)
  {
    if (!m_overlaySizes.empty())
    {
      handle = make_unique_dp<DynamicSquareHandle>(overlayId, m_params.m_anchor, m_point, m_overlaySizes,
                                                   m2::PointD(m_params.m_offset), GetOverlayPriority(), true /* isBound */,
                                                   debugName, m_params.m_minVisibleScale, true /* isBillboard */);
    }
    else
    {
      handle = make_unique_dp<dp::SquareHandle>(overlayId, m_params.m_anchor, m_point, m2::PointD(pixelSize),
                                                m2::PointD(m_params.m_offset), GetOverlayPriority(), true /* isBound */,
                                                debugName, m_params.m_minVisibleScale, true /* isBillboard */);
    }

    if (m_params.m_specialDisplacement == SpecialDisplacement::UserMark ||
        m_params.m_specialDisplacement == SpecialDisplacement::SpecialModeUserMark)
    {
      handle->SetSpecialLayerOverlay(true);
    }
    handle->SetOverlayRank(m_params.m_startOverlayRank);
  }
  auto state = CreateRenderState(gpu::Program::ColoredSymbol, m_params.m_depthLayer);
  state.SetProgram3d(gpu::Program::ColoredSymbolBillboard);
  state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
  state.SetColorTexture(colorRegion.GetTexture());
  state.SetDepthFunction(dp::TestFunction::Less);

  dp::AttributeProvider provider(1, static_cast<uint32_t>(buffer.size()));
  provider.InitStream(0, gpu::ColoredSymbolVertex::GetBindingInfo(), make_ref(buffer.data()));
  batcher->InsertTriangleList(context, state, make_ref(&provider), move(handle));
}

uint64_t ColoredSymbolShape::GetOverlayPriority() const
{
  // Special displacement mode.
  if (m_params.m_specialDisplacement == SpecialDisplacement::SpecialMode)
    return dp::CalculateSpecialModePriority(m_params.m_specialPriority);

  if (m_params.m_specialDisplacement == SpecialDisplacement::SpecialModeUserMark)
    return dp::CalculateSpecialModeUserMarkPriority(m_params.m_specialPriority);

  if (m_params.m_specialDisplacement == SpecialDisplacement::UserMark)
    return dp::CalculateUserMarkPriority(m_params.m_minVisibleScale, m_params.m_specialPriority);

  return dp::CalculateOverlayPriority(m_params.m_minVisibleScale, m_params.m_rank, m_params.m_depth);
}
}  // namespace df
