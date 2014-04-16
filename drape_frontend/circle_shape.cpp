#include "circle_shape.hpp"

#include "../drape/batcher.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/shader_def.hpp"

#define BLOCK_X_OFFSET  0
#define BLOCK_Y_OFFSET  1
#define BLOCK_Z_OFFSET  2
#define BLOCK_NX_OFFSET 3
#define BLOCK_NY_OFFSET 4

namespace df
{

namespace
{

void AddPoint(vector<float> & stream, size_t pointIndex, float x, float y, float z, float nX, float nY)
{
  size_t startIndex = pointIndex * 5;
  stream[startIndex + BLOCK_X_OFFSET] = x;
  stream[startIndex + BLOCK_Y_OFFSET] = y;
  stream[startIndex + BLOCK_Z_OFFSET] = z;
  stream[startIndex + BLOCK_NX_OFFSET] = nX;
  stream[startIndex + BLOCK_NY_OFFSET] = nY;
}

} // namespace

CircleShape::CircleShape(m2::PointF const & mercatorPt, CircleViewParams const & params)
  : m_pt(mercatorPt)
  , m_params(params)
{
}

void CircleShape::Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder>) const
{
  double const TriangleCount = 20.0;
  double const etalonSector = (2.0 * math::pi) / TriangleCount;

  /// x, y, z floats on geompoint
  /// normal x, y on triangle forming normals
  /// 20 triangles on full circle
  /// 22 points as triangle fan need n + 2 points on n triangles
  vector<float> stream((3 + 2) * (TriangleCount + 2));
  AddPoint(stream, 0, m_pt.x, m_pt.y, m_params.m_depth, 0.0f, 0.0f);

  m2::PointD startNormal(0.0f, m_params.m_radius);

  for (size_t i = 0; i < TriangleCount + 1; ++i)
  {
    m2::PointD rotatedNormal = m2::Rotate(startNormal, (i) * etalonSector);
    AddPoint(stream, i + 1, m_pt.x, m_pt.y, m_params.m_depth, rotatedNormal.x, rotatedNormal.y);
  }

  GLState state(gpu::SOLID_SHAPE_PROGRAM, GLState::OverlayLayer);
  state.SetColor(m_params.m_color);

  AttributeProvider provider(1, TriangleCount + 2);
  BindingInfo info(2);
  BindingDecl & posDecl = info.GetBindingDecl(0);
  posDecl.m_attributeName = "a_position";
  posDecl.m_componentCount = 3;
  posDecl.m_componentType = gl_const::GLFloatType;
  posDecl.m_offset = 0;
  posDecl.m_stride = 5 * sizeof(float);

  BindingDecl & normalDecl = info.GetBindingDecl(1);
  normalDecl.m_attributeName = "a_normal";
  normalDecl.m_componentCount = 2;
  normalDecl.m_componentType = gl_const::GLFloatType;
  normalDecl.m_offset = 3 * sizeof(float);
  normalDecl.m_stride = 5 * sizeof(float);

  OverlayHandle * overlay = new OverlayHandle(m_params.m_id,
                                              OverlayHandle::Center, m_pt,
                                              m2::PointD(m_params.m_radius, m_params.m_radius),
                                              m_params.m_depth);

  provider.InitStream(0, info, MakeStackRefPointer<void>(&stream[0]));
  batcher->InsertTriangleFan(state, MakeStackRefPointer(&provider), MovePointer(overlay));
}

} // namespace df
