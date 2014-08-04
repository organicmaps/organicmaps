#include "line_shape.hpp"

#include "../drape/shader_def.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"

#include "../base/math.hpp"
#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"
#include "../std/vector.hpp"

using m2::PointF;

namespace df
{

namespace
{
  static uint32_t const ListStride = 24;
}

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

template <typename T>
void QuadStripToList(vector<T> & dst, vector<T> & src, int32_t index)
{
  static const int32_t dstStride = 6;
  static const int32_t srcStride = 4;
  const int32_t baseDstIndex = index * dstStride;
  const int32_t baseSrcIndex = index * srcStride;

  dst[baseDstIndex] = src[baseSrcIndex];
  dst[baseDstIndex + 1] = src[baseSrcIndex + 1];
  dst[baseDstIndex + 2] = src[baseSrcIndex + 2];
  dst[baseDstIndex + 3] = src[baseSrcIndex + 1];
  dst[baseDstIndex + 4] = src[baseSrcIndex + 3];
  dst[baseDstIndex + 5] = src[baseSrcIndex + 2];
}

void SetColor(vector<float> &dst, float const * ar, int index)
{
  uint32_t const colorArraySize = 4;
  uint32_t const baseListIndex = ListStride * index;
  for (uint32_t i = 0; i < 6; ++i)
    memcpy(&dst[baseListIndex + colorArraySize * i], ar, colorArraySize * sizeof(float));
}

struct Vertex
{
  Vertex() {}
  Vertex(m2::PointF const & pos, m2::PointF const & dir)
    : m_position(pos), m_direction(dir) {}

  Vertex(float posX, float posY, float dirX = 0.0f, float dirY = 0.0f)
    : m_position(posX, posY), m_direction(dirX, dirY) {}

  m2::PointF m_position;
  m2::PointF m_direction;
};

struct Offset
{
  Offset() : m_topOffset(0.0f), m_pointSide(0.0f), m_depth(0.0f) {}
  Offset(float topOffset, float pointSide, float depth)
    : m_topOffset(topOffset), m_pointSide(pointSide), m_depth(depth) {}

  float m_topOffset;
  float m_pointSide;
  float m_depth;
};

struct WidthType
{
  WidthType() {}
  WidthType(float halfWidth, float cap, float join, float insetsWidth)
    : m_halfWidth(halfWidth), m_capType(cap), m_joinType(join), m_insetsWidth(insetsWidth) {}

  float m_halfWidth;
  float m_capType;
  float m_joinType;
  float m_insetsWidth;
};

struct SphereCenters
{
  SphereCenters() {}
  SphereCenters(m2::PointF const & center1, m2::PointF const & center2)
    : m_center1(center1), m_center2(center2) {}

  m2::PointF m_center1;
  m2::PointF m_center2;
};

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

void LineShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> /*textures*/) const
{
  int size = m_points.size();
  float const r = 1.0f;

  int const numVert = (size - 1) * 4;
  vector<Vertex> vertex(numVert);
  vector<Offset> dxVals(numVert);
  vector<SphereCenters> centers(numVert);
  vector<WidthType> widthType(numVert);

  PointF leftBisector, rightBisector, dx;

  PointF v2 = m_points[0];
  PointF v3 = m_points[1];
  PointF v1 = v2 * 2 - v3;

  Bisector(r, v1, v2, v3, leftBisector, rightBisector, dx);

  float const joinType = m_params.m_join == dp::RoundJoin ? 1 : 0;
  float const halfWidth = m_params.m_width / 2.0f;
  float const insetHalfWidth= 1.0f * halfWidth;

  vertex[0] = Vertex(v2, leftBisector);
  vertex[1] = Vertex(v2, rightBisector);
  dxVals[0] = Offset(0.0f, -1.0f, m_params.m_depth);
  dxVals[1] = Offset(0.0f, -1.0f, m_params.m_depth);
  centers[0] = centers[1] = SphereCenters(v2, v3);

  widthType[0] = widthType[2] = WidthType(halfWidth, 0, joinType, insetHalfWidth);
  widthType[1] = widthType[3] = WidthType(-halfWidth, 0, joinType, insetHalfWidth);

  //points in the middle
  for(int i = 1 ; i < size - 1 ; ++i)
  {
    v1 = v2;
    v2 = v3;
    v3 = m_points[i + 1];
    Bisector(r, v1, v2, v3, leftBisector, rightBisector, dx);
    float aspect = (v1-v2).Length() / (v2-v3).Length();

    vertex[(i-1) * 4 + 2] = Vertex(v2, leftBisector);
    vertex[(i-1) * 4 + 3] = Vertex(v2, rightBisector);
    dxVals[(i-1) * 4 + 2] = Offset(dx.x, 1.0f, m_params.m_depth);
    dxVals[(i-1) * 4 + 3] = Offset(-dx.x, 1.0f, m_params.m_depth);
    centers[(i-1) * 4 + 2] = centers[(i-1) * 4 + 3] = SphereCenters(v1, v2);

    vertex[i * 4 + 0] = Vertex(v2, leftBisector);
    vertex[i * 4 + 1] = Vertex(v2, rightBisector);
    dxVals[i * 4 + 0] = Offset(-dx.x * aspect, -1.0f, m_params.m_depth);
    dxVals[i * 4 + 1] = Offset(dx.x * aspect, -1.0f, m_params.m_depth);
    centers[i * 4] = centers[i * 4 + 1] = SphereCenters(v2, v3);

    widthType[(i * 4) + 0] = widthType[(i * 4) + 2] = WidthType(halfWidth, 0, joinType, insetHalfWidth);
    widthType[(i * 4) + 1] = widthType[(i * 4) + 3] = WidthType(-halfWidth, 0, joinType, insetHalfWidth);
  }

  //last points
  v1 = v2;
  v2 = v3;
  v3 = v2 * 2 - v1;

  Bisector(r, v1, v2, v3, leftBisector, rightBisector, dx);
  float const aspect = (v1-v2).Length() / (v2-v3).Length();

  vertex[(size - 2) * 4 + 2] = Vertex(v2, leftBisector);
  vertex[(size - 2) * 4 + 3] = Vertex(v2, rightBisector);
  dxVals[(size - 2) * 4 + 2] = Offset(-dx.x * aspect, 1.0f, m_params.m_depth);
  dxVals[(size - 2) * 4 + 3] = Offset(dx.x * aspect, 1.0f, m_params.m_depth);
  widthType[(size - 2) * 4] = widthType[(size - 2) * 4 + 2] = WidthType(halfWidth, 0, joinType, insetHalfWidth);
  widthType[(size - 2) * 4 + 1] = widthType[(size - 2) * 4 + 3] = WidthType(-halfWidth, 0, joinType, insetHalfWidth);
  centers[(size - 2) * 4 + 2] = centers[(size - 2) * 4 + 3] = SphereCenters(v1, v2);

  if (m_params.m_cap != dp::ButtCap)
  {
    float const type = m_params.m_cap == dp::RoundCap ? -1 : 1;
    uint32_t const baseIdx = 4 * (size - 2);
    widthType[0] = WidthType(halfWidth, type, -1, insetHalfWidth);
    widthType[1] = WidthType(-halfWidth, type, -1, insetHalfWidth);
    widthType[2] = WidthType(halfWidth, type, -1, insetHalfWidth);
    widthType[3] = WidthType(-halfWidth, type, -1, insetHalfWidth);

    widthType[baseIdx] = WidthType(halfWidth, type, 1, insetHalfWidth);
    widthType[baseIdx + 1] = WidthType(-halfWidth, type, 1, insetHalfWidth);
    widthType[baseIdx + 2] = WidthType(halfWidth, type, 1, insetHalfWidth);
    widthType[baseIdx + 3] = WidthType(-halfWidth, type, 1, insetHalfWidth);

    vertex[0].m_position = vertex[2].m_position;
    vertex[1].m_position = vertex[3].m_position;
    vertex[baseIdx + 2].m_position = vertex[baseIdx].m_position;
    vertex[baseIdx + 3].m_position = vertex[baseIdx + 1].m_position;
  }

  float clr1[4];
  Convert(m_params.m_color, clr1[0], clr1[1], clr1[2], clr1[3]);
  /// TODO this color now not using. We need merge line styles to draw line outline and line by ont pass
  float const clr2[4] = {0.5f, 0.5f, 0.5f, 1.0f};

  /// TODO add additional functionality in batcher for better perfomance
  int32_t const listVertexCount = (numVert >> 1) * 3;
  vector<Vertex> vertex2(listVertexCount);
  vector<Offset> dxVals2(listVertexCount);
  vector<SphereCenters> centers2(listVertexCount);
  vector<WidthType> widthType2(listVertexCount);
  vector<float> baseColor(numVert * 6);
  vector<float> outlineColor(numVert * 6);
  for(int i = 0; i < size-1 ; i++)
  {
    QuadStripToList(vertex2, vertex, i);
    QuadStripToList(dxVals2, dxVals, i);
    QuadStripToList(centers2, centers, i);
    QuadStripToList(widthType2, widthType, i);
    SetColor(baseColor, clr1, i);
    SetColor(outlineColor, clr2, i);
  }

  dp::GLState state(gpu::SOLID_LINE_PROGRAM, dp::GLState::GeometryLayer);
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
    decl.m_attributeName = "color1";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(4, clr1, dp::MakeStackRefPointer((void*)&baseColor[0]));
  }

  {
    dp::BindingInfo clr2(1);
    dp::BindingDecl & decl = clr2.GetBindingDecl(0);
    decl.m_attributeName = "color2";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(5, clr2, dp::MakeStackRefPointer((void*)&outlineColor[0]));
  }

  //batcher->InsertTriangleList(state, dp::MakeStackRefPointer(&provider));
  batcher->InsertListOfStrip(state, dp::MakeStackRefPointer(&provider), 4);
}

} // namespace df

