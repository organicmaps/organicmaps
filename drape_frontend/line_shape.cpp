#include "line_shape.hpp"
#include "common_structures.hpp"

#include "../drape/shader_def.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"
#include "../drape/texture_of_colors.hpp"
#include "../drape/texture_set_holder.hpp"

#include "../base/math.hpp"
#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"
#include "../std/vector.hpp"

using m2::PointF;

namespace df
{

using glsl_types::vec2;
using glsl_types::vec3;
using glsl_types::vec4;

/// Split angle v1-v2-v3 by bisector.
void Bisector(float R, PointF const & v1, PointF const & v2, PointF const & v3,
              PointF & leftBisector, PointF & rightBisector, PointF & dx)
{
  PointF const dif21 = v2 - v1;
  PointF const dif32 = v3 - v2;

  float const l1 = dif21.Length();
  float const l2 = dif32.Length();
  float const l3 = (v1 - v3).Length();

  /// normal1 is normal from segment v2 - v1
  /// normal2 is normal from segmeny v3 - v2
  PointF normal1(-dif21.y / l1, dif21.x / l1);
  PointF const normal2(-dif32.y / l2, dif32.x / l2);

  leftBisector = normal1 + normal2;

  /// find cos(2a), where angle a is a half of v1-v2-v3 angle
  float const cos2A = (l1 * l1 + l2 * l2 - l3 * l3) / (2.0f * l1 * l2);
  float const sinA = sqrt((1.0f - cos2A) / 2.0f);
  float const radius = R / sinA;

  leftBisector = (leftBisector * radius) / leftBisector.Length();
  rightBisector = -leftBisector;

  normal1 = normal1 * R;
  dx = leftBisector - normal1;

  float const sign = (m2::DotProduct(dif21, leftBisector) > 0.0f) ? 1.0f : -1.0f;
  dx.x = 2.0f * sign * (dx.Length() / l1);

  leftBisector += v2;
  rightBisector += v2;
}

LineShape::LineShape(vector<m2::PointF> const & points,
                     LineViewParams const & params)
  : m_params(params)
  , m_points(params.m_cap == dp::ButtCap ? points.size() : points.size() + 2)
{
  ASSERT_GREATER(points.size(), 1, ());

  int const size = m_points.size();
  if (m_params.m_cap != dp::ButtCap)
  {
    m_points[0] = points[0] + (points[0] - points[1]).Normalize();;
    m_points[size - 1] = points[size - 3] + (points[size - 3] - points[size - 4]).Normalize();
    memcpy(&m_points[1], &points[0], (size - 2) * sizeof(PointF));
  }
  else
    m_points = points;
}

void LineShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  int size = m_points.size();
  float const r = 1.0f;

  int const numVert = (size - 1) * 4;
  vector<vec4> vertex(numVert);
  vector<vec3> dxVals(numVert);
  vector<vec4> centers(numVert);
  vector<vec4> widthType(numVert);

  PointF leftBisector, rightBisector, dx;

  PointF v2 = m_points[0];
  PointF v3 = m_points[1];
  PointF v1 = v2 * 2 - v3;

  Bisector(r, v1, v2, v3, leftBisector, rightBisector, dx);

  float const joinType = m_params.m_join == dp::RoundJoin ? 1 : 0;
  float const halfWidth = m_params.m_width / 2.0f;
  float const insetHalfWidth= 1.0f * halfWidth;

  vertex[0] = vec4(v2, leftBisector);
  vertex[1] = vec4(v2, rightBisector);
  dxVals[0] = vec3(0.0f, -1.0f, m_params.m_depth);
  dxVals[1] = vec3(0.0f, -1.0f, m_params.m_depth);
  centers[0] = centers[1] = vec4(v2, v3);

  widthType[0] = widthType[2] = vec4(halfWidth, 0, joinType, insetHalfWidth);
  widthType[1] = widthType[3] = vec4(-halfWidth, 0, joinType, insetHalfWidth);

  //points in the middle
  for(int i = 1 ; i < size - 1 ; ++i)
  {
    v1 = v2;
    v2 = v3;
    v3 = m_points[i + 1];
    Bisector(r, v1, v2, v3, leftBisector, rightBisector, dx);
    float aspect = (v1-v2).Length() / (v2-v3).Length();

    vertex[(i-1) * 4 + 2] = vec4(v2, leftBisector);
    vertex[(i-1) * 4 + 3] = vec4(v2, rightBisector);
    dxVals[(i-1) * 4 + 2] = vec3(dx.x, 1.0f, m_params.m_depth);
    dxVals[(i-1) * 4 + 3] = vec3(-dx.x, 1.0f, m_params.m_depth);
    centers[(i-1) * 4 + 2] = centers[(i-1) * 4 + 3] = vec4(v1, v2);

    vertex[i * 4 + 0] = vec4(v2, leftBisector);
    vertex[i * 4 + 1] = vec4(v2, rightBisector);
    dxVals[i * 4 + 0] = vec3(-dx.x * aspect, -1.0f, m_params.m_depth);
    dxVals[i * 4 + 1] = vec3(dx.x * aspect, -1.0f, m_params.m_depth);
    centers[i * 4] = centers[i * 4 + 1] = vec4(v2, v3);

    widthType[(i * 4) + 0] = widthType[(i * 4) + 2] = vec4(halfWidth, 0, joinType, insetHalfWidth);
    widthType[(i * 4) + 1] = widthType[(i * 4) + 3] = vec4(-halfWidth, 0, joinType, insetHalfWidth);
  }

  //last points
  v1 = v2;
  v2 = v3;
  v3 = v2 * 2 - v1;

  Bisector(r, v1, v2, v3, leftBisector, rightBisector, dx);
  float const aspect = (v1-v2).Length() / (v2-v3).Length();

  vertex[(size - 2) * 4 + 2] = vec4(v2, leftBisector);
  vertex[(size - 2) * 4 + 3] = vec4(v2, rightBisector);
  dxVals[(size - 2) * 4 + 2] = vec3(-dx.x * aspect, 1.0f, m_params.m_depth);
  dxVals[(size - 2) * 4 + 3] = vec3(dx.x * aspect, 1.0f, m_params.m_depth);
  widthType[(size - 2) * 4] = widthType[(size - 2) * 4 + 2] = vec4(halfWidth, 0, joinType, insetHalfWidth);
  widthType[(size - 2) * 4 + 1] = widthType[(size - 2) * 4 + 3] = vec4(-halfWidth, 0, joinType, insetHalfWidth);
  centers[(size - 2) * 4 + 2] = centers[(size - 2) * 4 + 3] = vec4(v1, v2);

  if (m_params.m_cap != dp::ButtCap)
  {
    float const type = m_params.m_cap == dp::RoundCap ? -1 : 1;
    uint32_t const baseIdx = 4 * (size - 2);
    widthType[0] = vec4(halfWidth, type, -1, insetHalfWidth);
    widthType[1] = vec4(-halfWidth, type, -1, insetHalfWidth);
    widthType[2] = vec4(halfWidth, type, -1, insetHalfWidth);
    widthType[3] = vec4(-halfWidth, type, -1, insetHalfWidth);

    widthType[baseIdx] = vec4(halfWidth, type, 1, insetHalfWidth);
    widthType[baseIdx + 1] = vec4(-halfWidth, type, 1, insetHalfWidth);
    widthType[baseIdx + 2] = vec4(halfWidth, type, 1, insetHalfWidth);
    widthType[baseIdx + 3] = vec4(-halfWidth, type, 1, insetHalfWidth);

    vertex[0].x = vertex[2].x;
    vertex[0].y = vertex[2].y;
    vertex[1].x = vertex[3].x;
    vertex[1].y = vertex[3].y;
    vertex[baseIdx + 2].x = vertex[baseIdx].x;
    vertex[baseIdx + 2].y = vertex[baseIdx].y;
    vertex[baseIdx + 3].x = vertex[baseIdx + 1].x;
    vertex[baseIdx + 3].y = vertex[baseIdx + 1].y;
  }

  dp::ColorKey key(m_params.m_color.GetColorInInt());
  dp::TextureSetHolder::ColorRegion region;
  textures->GetColorRegion(key, region);
  m2::RectF const & rect = region.GetTexRect();
  key.SetColor(dp::Color(127, 127, 127, 255).GetColorInInt());
  textures->GetColorRegion(key, region);
  m2::RectF const & outlineRect = region.GetTexRect();
  float texIndex = static_cast<float>(region.GetTextureNode().m_textureOffset);

  vector<vec4> colors(numVert, vec4(rect.RightTop(), outlineRect.RightTop()));
  vector<float> index(numVert, texIndex); /// TODO this color now not using.
  ///We need merge line styles to draw line outline and line by ont pass

  dp::GLState state(gpu::SOLID_LINE_PROGRAM, dp::GLState::GeometryLayer);
  state.SetTextureSet(region.GetTextureNode().m_textureSet);
  state.SetBlending(dp::Blending(true));

  dp::AttributeProvider provider(6, 4 * (size - 1));

  {
    dp::BindingInfo pos_dir(1);
    dp::BindingDecl & decl = pos_dir.GetBindingDecl(0);
    decl.m_attributeName = "position";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(0, pos_dir, dp::MakeStackRefPointer((void*)&vertex[0]));
  }
  {
    dp::BindingInfo deltas(1);
    dp::BindingDecl & decl = deltas.GetBindingDecl(0);
    decl.m_attributeName = "deltas";
    decl.m_componentCount = 3;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(1, deltas, dp::MakeStackRefPointer((void*)&dxVals[0]));
  }
  {
    dp::BindingInfo width_type(1);
    dp::BindingDecl & decl = width_type.GetBindingDecl(0);
    decl.m_attributeName = "width_type";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(2, width_type, dp::MakeStackRefPointer((void*)&widthType[0]));
  }
  {
    dp::BindingInfo centres(1);
    dp::BindingDecl & decl = centres.GetBindingDecl(0);
    decl.m_attributeName = "centres";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(3, centres, dp::MakeStackRefPointer((void*)&centers[0]));
  }

  {
    dp::BindingInfo clr1(1);
    dp::BindingDecl & decl = clr1.GetBindingDecl(0);
    decl.m_attributeName = "colors";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(4, clr1, dp::MakeStackRefPointer((void*)&colors[0]));
  }

  {
    dp::BindingInfo ind(1);
    dp::BindingDecl & decl = ind.GetBindingDecl(0);
    decl.m_attributeName = "index";
    decl.m_componentCount = 1;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(5, ind, dp::MakeStackRefPointer((void*)&index[0]));
  }

  batcher->InsertListOfStrip(state, dp::MakeStackRefPointer(&provider), 4);
}

} // namespace df

