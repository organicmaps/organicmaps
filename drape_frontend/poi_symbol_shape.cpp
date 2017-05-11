#include "drape_frontend/poi_symbol_shape.hpp"
#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/shader_def.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

namespace
{
df::ColorConstant const kPoiDeletedMaskColor = "PoiDeletedMask";

using SV = gpu::SolidTexturingVertex;
using MV = gpu::MaskedTexturingVertex;

template<typename TVertex>
void Batch(ref_ptr<dp::Batcher> batcher, drape_ptr<dp::OverlayHandle> && handle,
           glsl::vec4 const & position,
           dp::TextureManager::SymbolRegion const & symbolRegion,
           dp::TextureManager::ColorRegion const & colorRegion)
{
  ASSERT(0, ("Can not be used without specialization"));
}

template<>
void Batch<SV>(ref_ptr<dp::Batcher> batcher, drape_ptr<dp::OverlayHandle> && handle,
               glsl::vec4 const & position,
               dp::TextureManager::SymbolRegion const & symbolRegion,
               dp::TextureManager::ColorRegion const & colorRegion)
{
  m2::PointF const pixelSize = symbolRegion.GetPixelSize();
  m2::PointF const halfSize(pixelSize.x * 0.5f, pixelSize.y * 0.5f);
  m2::RectF const & texRect = symbolRegion.GetTexRect();

  SV vertexes[] =
  {
    SV{ position, glsl::vec2(-halfSize.x, halfSize.y),
        glsl::vec2(texRect.minX(), texRect.maxY()) },
    SV{ position, glsl::vec2(-halfSize.x, -halfSize.y),
        glsl::vec2(texRect.minX(), texRect.minY()) },
    SV{ position, glsl::vec2(halfSize.x, halfSize.y),
        glsl::vec2(texRect.maxX(), texRect.maxY()) },
    SV{ position, glsl::vec2(halfSize.x, -halfSize.y),
        glsl::vec2(texRect.maxX(), texRect.minY()) },
  };

  dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::OverlayLayer);
  state.SetProgram3dIndex(gpu::TEXTURING_BILLBOARD_PROGRAM);
  state.SetColorTexture(symbolRegion.GetTexture());
  state.SetTextureFilter(gl_const::GLNearest);

  dp::AttributeProvider provider(1 /* streamCount */, ARRAY_SIZE(vertexes));
  provider.InitStream(0 /* streamIndex */, SV::GetBindingInfo(), make_ref(vertexes));
  batcher->InsertTriangleStrip(state, make_ref(&provider), move(handle));
}

template<>
void Batch<MV>(ref_ptr<dp::Batcher> batcher, drape_ptr<dp::OverlayHandle> && handle,
               glsl::vec4 const & position,
               dp::TextureManager::SymbolRegion const & symbolRegion,
               dp::TextureManager::ColorRegion const & colorRegion)
{
  m2::PointF const pixelSize = symbolRegion.GetPixelSize();
  m2::PointF const halfSize(pixelSize.x * 0.5f, pixelSize.y * 0.5f);
  m2::RectF const & texRect = symbolRegion.GetTexRect();
  glsl::vec2 const maskColorCoords = glsl::ToVec2(colorRegion.GetTexRect().Center());

  MV vertexes[] =
  {
    MV{ position, glsl::vec2(-halfSize.x, halfSize.y),
        glsl::vec2(texRect.minX(), texRect.maxY()), maskColorCoords },
    MV{ position, glsl::vec2(-halfSize.x, -halfSize.y),
        glsl::vec2(texRect.minX(), texRect.minY()), maskColorCoords },
    MV{ position, glsl::vec2(halfSize.x, halfSize.y),
        glsl::vec2(texRect.maxX(), texRect.maxY()), maskColorCoords },
    MV{ position, glsl::vec2(halfSize.x, -halfSize.y),
        glsl::vec2(texRect.maxX(), texRect.minY()), maskColorCoords },
  };

  dp::GLState state(gpu::MASKED_TEXTURING_PROGRAM, dp::GLState::OverlayLayer);
  state.SetProgram3dIndex(gpu::MASKED_TEXTURING_BILLBOARD_PROGRAM);
  state.SetColorTexture(symbolRegion.GetTexture());
  state.SetMaskTexture(colorRegion.GetTexture()); // Here mask is a color.
  state.SetTextureFilter(gl_const::GLNearest);

  dp::AttributeProvider provider(1 /* streamCount */, ARRAY_SIZE(vertexes));
  provider.InitStream(0 /* streamIndex */, MV::GetBindingInfo(), make_ref(vertexes));
  batcher->InsertTriangleStrip(state, make_ref(&provider), move(handle));
}

} // namespace

namespace df
{
PoiSymbolShape::PoiSymbolShape(m2::PointD const & mercatorPt, PoiSymbolViewParams const & params,
                               TileKey const & tileKey, uint32_t textIndex,
                               int displacementMode, uint16_t specialModePriority)
  : m_pt(mercatorPt)
  , m_params(params)
  , m_displacementMode(displacementMode)
  , m_specialModePriority(specialModePriority)
  , m_tileCoords(tileKey.GetTileCoords())
  , m_textIndex(textIndex)
{}

void PoiSymbolShape::Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
{
  dp::TextureManager::SymbolRegion region;
  textures->GetSymbolRegion(m_params.m_symbolName, region);

  glsl::vec2 const pt = glsl::ToVec2(ConvertToLocal(m_pt, m_params.m_tileCenter, kShapeCoordScalar));
  glsl::vec4 const position = glsl::vec4(pt, m_params.m_depth, -m_params.m_posZ);
  m2::PointF const pixelSize = region.GetPixelSize();

  dp::OverlayID overlayId = dp::OverlayID(m_params.m_id, m_tileCoords, m_textIndex);
  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<dp::SquareHandle>(overlayId,
                                                                         dp::Center,
                                                                         m_pt, pixelSize,
                                                                         GetOverlayPriority(),
                                                                         true /* isBound */,
                                                                         m_params.m_symbolName,
                                                                         true /* isBillboard */);
  handle->SetDisplacementMode(m_displacementMode);
  handle->SetPivotZ(m_params.m_posZ);
  handle->SetExtendingSize(m_params.m_extendingSize);

  if (m_params.m_obsoleteInEditor)
  {
    dp::TextureManager::ColorRegion maskColorRegion;
    textures->GetColorRegion(df::GetColorConstant(kPoiDeletedMaskColor), maskColorRegion);
    Batch<MV>(batcher, move(handle), position, region, maskColorRegion);
  }
  else
  {
    Batch<SV>(batcher, move(handle), position, region, dp::TextureManager::ColorRegion());
  }
}

uint64_t PoiSymbolShape::GetOverlayPriority() const
{
  // Set up maximum priority for some POI.
  if (m_params.m_prioritized)
    return dp::kPriorityMaskAll;

  // Special displacement mode.
  if ((m_displacementMode & dp::displacement::kDefaultMode) == 0)
    return dp::CalculateSpecialModePriority(m_specialModePriority);

  // Set up minimal priority for shapes which belong to areas.
  if (m_params.m_hasArea)
    return 0;

  return dp::CalculateOverlayPriority(m_params.m_minVisibleScale, m_params.m_rank, m_params.m_depth);
}

} // namespace df
