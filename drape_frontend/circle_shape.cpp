#include "drape_frontend/circle_shape.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/batcher.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/glstate.hpp"
#include "drape/shader_def.hpp"
#include "drape/texture_manager.hpp"

namespace df
{

CircleShape::CircleShape(m2::PointF const & mercatorPt, CircleViewParams const & params)
  : m_pt(mercatorPt)
  , m_params(params)
{
}

void CircleShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureManager> textures) const
{
  int const TriangleCount = 20;
  double const etalonSector = (2.0 * math::pi) / static_cast<double>(TriangleCount);

  dp::TextureManager::ColorRegion region;
  textures->GetColorRegion(m_params.m_color, region);
  glsl::vec2 colorPoint(glsl::ToVec2(region.GetTexRect().Center()));

  buffer_vector<gpu::SolidTexturingVertex, 22> vertexes;
  vertexes.push_back(gpu::SolidTexturingVertex
  {
    glsl::vec3(glsl::ToVec2(m_pt), m_params.m_depth),
    glsl::vec2(0.0f, 0.0f),
    colorPoint
  });

  m2::PointD startNormal(0.0f, m_params.m_radius);

  for (size_t i = 0; i < TriangleCount + 1; ++i)
  {
    m2::PointD rotatedNormal = m2::Rotate(startNormal, (i) * etalonSector);
    vertexes.push_back(gpu::SolidTexturingVertex
    {
      glsl::vec3(glsl::ToVec2(m_pt), m_params.m_depth),
      glsl::ToVec2(rotatedNormal),
      colorPoint
    });
  }

  dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::OverlayLayer);
  state.SetBlending(dp::Blending(true));
  state.SetColorTexture(region.GetTexture());

  double handleSize = 2 * m_params.m_radius;

  dp::OverlayHandle * overlay = new dp::SquareHandle(m_params.m_id,
                                                     dp::Center, m_pt,
                                                     m2::PointD(handleSize, handleSize),
                                                     m_params.m_depth);

  dp::AttributeProvider provider(1, TriangleCount + 2);
  provider.InitStream(0, gpu::SolidTexturingVertex::GetBindingInfo(), dp::MakeStackRefPointer<void>(vertexes.data()));
  batcher->InsertTriangleFan(state, dp::MakeStackRefPointer(&provider), dp::MovePointer(overlay));
}

} // namespace df
