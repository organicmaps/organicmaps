#include "circle_shape.hpp"
#include "common_structures.hpp"

#include "../drape/batcher.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/shader_def.hpp"
#include "../drape/texture_of_colors.hpp"
#include "../drape/texture_set_holder.hpp"

#define BLOCK_X_OFFSET  0
#define BLOCK_Y_OFFSET  1
#define BLOCK_Z_OFFSET  2
#define BLOCK_NX_OFFSET 3
#define BLOCK_NY_OFFSET 4
#define BLOCK_CX_OFFSET 5
#define BLOCK_CY_OFFSET 6
#define BLOCK_IND_OFFSET 7
#define VERTEX_STRIDE 8

namespace df
{
  using glsl_types::vec2;
  using glsl_types::vec3;
  using glsl_types::vec4;

namespace
{

void AddPoint(vector<float> & stream, size_t pointIndex, float x, float y, float z, float nX, float nY, vec3 const & color)
{
  size_t startIndex = pointIndex * VERTEX_STRIDE;
  stream[startIndex + BLOCK_X_OFFSET] = x;
  stream[startIndex + BLOCK_Y_OFFSET] = y;
  stream[startIndex + BLOCK_Z_OFFSET] = z;
  stream[startIndex + BLOCK_NX_OFFSET] = nX;
  stream[startIndex + BLOCK_NY_OFFSET] = nY;
  stream[startIndex + BLOCK_CX_OFFSET] = color.x;
  stream[startIndex + BLOCK_CY_OFFSET] = color.y;
  stream[startIndex + BLOCK_IND_OFFSET] = color.z;
}

} // namespace

CircleShape::CircleShape(m2::PointF const & mercatorPt, CircleViewParams const & params)
  : m_pt(mercatorPt)
  , m_params(params)
{
}

void CircleShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  double const TriangleCount = 20.0;
  double const etalonSector = (2.0 * math::pi) / TriangleCount;

  dp::ColorKey key(m_params.m_color.GetColorInInt());
  dp::TextureSetHolder::ColorRegion region;
  textures->GetColorRegion(key, region);
  m2::RectF const & rect = region.GetTexRect();
  float texIndex = static_cast<float>(region.GetTextureNode().m_textureOffset);

  vec3 color(rect.RightTop(), texIndex);

  /// x, y, z floats on geompoint
  /// normal x, y on triangle forming normals
  /// 20 triangles on full circle
  /// 22 points as triangle fan need n + 2 points on n triangles

  vector<float> stream((3 + 2 + 3) * (TriangleCount + 2));
  AddPoint(stream, 0, m_pt.x, m_pt.y, m_params.m_depth, 0.0f, 0.0f, color);

  m2::PointD startNormal(0.0f, m_params.m_radius);

  for (size_t i = 0; i < TriangleCount + 1; ++i)
  {
    m2::PointD rotatedNormal = m2::Rotate(startNormal, (i) * etalonSector);
    AddPoint(stream, i + 1, m_pt.x, m_pt.y, m_params.m_depth, rotatedNormal.x, rotatedNormal.y, color);
  }

  dp::GLState state(gpu::SOLID_SHAPE_PROGRAM, dp::GLState::OverlayLayer);
  state.SetTextureSet(region.GetTextureNode().m_textureSet);

  dp::AttributeProvider provider(1, TriangleCount + 2);
  dp::BindingInfo info(3);
  dp::BindingDecl & posDecl = info.GetBindingDecl(0);
  posDecl.m_attributeName = "a_position";
  posDecl.m_componentCount = 3;
  posDecl.m_componentType = gl_const::GLFloatType;
  posDecl.m_offset = 0;
  posDecl.m_stride = VERTEX_STRIDE * sizeof(float);

  dp::BindingDecl & normalDecl = info.GetBindingDecl(1);
  normalDecl.m_attributeName = "a_normal";
  normalDecl.m_componentCount = 2;
  normalDecl.m_componentType = gl_const::GLFloatType;
  normalDecl.m_offset = BLOCK_NX_OFFSET * sizeof(float);
  normalDecl.m_stride = VERTEX_STRIDE * sizeof(float);

  dp::BindingDecl & colorDecl = info.GetBindingDecl(2);
  colorDecl.m_attributeName = "a_color_index";
  colorDecl.m_componentCount = 3;
  colorDecl.m_componentType = gl_const::GLFloatType;
  colorDecl.m_offset = BLOCK_CX_OFFSET * sizeof(float);
  colorDecl.m_stride = VERTEX_STRIDE * sizeof(float);

  dp::OverlayHandle * overlay = new dp::SquareHandle(m_params.m_id,
                                                     dp::Center, m_pt,
                                                     m2::PointD(m_params.m_radius, m_params.m_radius),
                                                     m_params.m_depth);

  provider.InitStream(0, info, dp::MakeStackRefPointer<void>(&stream[0]));
  batcher->InsertTriangleFan(state, dp::MakeStackRefPointer(&provider), dp::MovePointer(overlay));
}

} // namespace df
