#include "drape_frontend/line_shape.hpp"

#include "drape_frontend/line_shape_helper.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/glsl_types.hpp"
#include "drape/glsl_func.hpp"
#include "drape/shader_def.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/glstate.hpp"
#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"

#include "base/logging.hpp"

namespace df
{

namespace
{
  using LV = gpu::LineVertex;
  using TGeometryBuffer = buffer_vector<gpu::LineVertex, 128>;

  class TextureCoordGenerator
  {
  public:
    TextureCoordGenerator(float const baseGtoPScale)
      : m_baseGtoPScale(baseGtoPScale)
    {
    }

    void SetRegion(dp::TextureManager::StippleRegion const & region, bool isSolid)
    {
      m_isSolid = isSolid;
      m_region = region;
      if (!m_isSolid)
        m_maskLength = static_cast<float>(m_region.GetMaskPixelLength());
    }

    float GetUvOffsetByDistance(float distance) const
    {
      return m_isSolid ? 1.0 : (distance * m_baseGtoPScale / m_maskLength);
    }

    glsl::vec2 GetTexCoordsByDistance(float distance) const
    {
      float normalizedOffset = min(GetUvOffsetByDistance(distance), 1.0f);
      return GetTexCoords(normalizedOffset);
    }

    glsl::vec2 GetTexCoords(float normalizedOffset) const
    {
      m2::RectF const & texRect = m_region.GetTexRect();
      if (m_isSolid)
        return glsl::ToVec2(texRect.Center());

      return glsl::vec2(texRect.minX() + normalizedOffset * texRect.SizeX(), texRect.Center().y);
    }

    float GetMaskLength() const { return m_maskLength; }
    bool IsSolid() const { return m_isSolid; }

  private:
    float const m_baseGtoPScale;
    dp::TextureManager::StippleRegion m_region;
    float m_maskLength = 0.0f;
    bool m_isSolid = true;
  };
}

LineShape::LineShape(m2::SharedSpline const & spline,
                     LineViewParams const & params)
  : m_params(params)
  , m_spline(spline)
{
  ASSERT_GREATER(m_spline->GetPath().size(), 1, ());
}

void LineShape::Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
{
  TGeometryBuffer geometry;
  TGeometryBuffer joinsGeometry;
  vector<m2::PointD> const & path = m_spline->GetPath();
  ASSERT(path.size() > 1, ());

  // set up uv-generator
  dp::TextureManager::ColorRegion colorRegion;
  textures->GetColorRegion(m_params.m_color, colorRegion);
  glsl::vec2 colorCoord(glsl::ToVec2(colorRegion.GetTexRect().Center()));

  TextureCoordGenerator texCoordGen(m_params.m_baseGtoPScale);
  dp::TextureManager::StippleRegion maskRegion;
  if (m_params.m_pattern.empty())
    textures->GetStippleRegion(dp::TextureManager::TStipplePattern{1}, maskRegion);
  else
    textures->GetStippleRegion(m_params.m_pattern, maskRegion);

  texCoordGen.SetRegion(maskRegion, m_params.m_pattern.empty());

  float const halfWidth = m_params.m_width / 2.0f;
  float const glbHalfWidth = halfWidth / m_params.m_baseGtoPScale;

  auto const getLineVertex = [&](LineSegment const & segment, glsl::vec3 const & pivot,
                                 glsl::vec2 const & normal, float offsetFromStart)
  {
    float distance = GetProjectionLength(pivot.xy() + glbHalfWidth * normal,
                                         segment.m_points[StartPoint],
                                         segment.m_points[EndPoint]) - offsetFromStart;

    return LV(pivot, halfWidth * normal, colorCoord, texCoordGen.GetTexCoordsByDistance(distance));
  };

  auto const generateTriangles = [&](glsl::vec3 const & pivot, vector<glsl::vec2> const & normals)
  {
    size_t const trianglesCount = normals.size() / 3;
    for (int j = 0; j < trianglesCount; j++)
    {
      glsl::vec2 const uv = texCoordGen.GetTexCoords(0.0f);
      joinsGeometry.push_back(LV(pivot, normals[3 * j], colorCoord, uv));
      joinsGeometry.push_back(LV(pivot, normals[3 * j + 1], colorCoord, uv));
      joinsGeometry.push_back(LV(pivot, normals[3 * j + 2], colorCoord, uv));
    }
  };

  // constuct segments
  vector<LineSegment> segments;
  segments.reserve(path.size() - 1);
  ConstructLineSegments(path, segments);

  // build geometry
  for (size_t i = 0; i < segments.size(); i++)
  {
    UpdateNormals(&segments[i], (i > 0) ? &segments[i - 1] : nullptr,
                 (i < segments.size() - 1) ? &segments[i + 1] : nullptr);

    // calculate number of steps to cover line segment
    float const initialGlobalLength = glsl::length(segments[i].m_points[EndPoint] - segments[i].m_points[StartPoint]);
    int steps = 1;
    float maskSize = initialGlobalLength;
    if (!texCoordGen.IsSolid())
    {
      float const leftWidth = glbHalfWidth * max(segments[i].m_leftWidthScalar[StartPoint].y,
                                                 segments[i].m_rightWidthScalar[StartPoint].y);
      float const rightWidth = glbHalfWidth * max(segments[i].m_leftWidthScalar[EndPoint].y,
                                                  segments[i].m_rightWidthScalar[EndPoint].y);
      float const effectiveLength = initialGlobalLength - leftWidth - rightWidth;
      if (effectiveLength > 0)
      {
        float const pixelLen = effectiveLength * m_params.m_baseGtoPScale;
        steps = static_cast<int>((pixelLen + texCoordGen.GetMaskLength() - 1) / texCoordGen.GetMaskLength());
        maskSize = effectiveLength / steps;
      }
    }

    // generate main geometry
    float currentSize = 0;
    glsl::vec3 currentStartPivot = glsl::vec3(segments[i].m_points[StartPoint], m_params.m_depth);
    for (int step = 0; step < steps; step++)
    {
      float const offsetFromStart = currentSize;
      currentSize += maskSize;

      glsl::vec2 const newPoint = segments[i].m_points[StartPoint] + segments[i].m_tangent * currentSize;
      glsl::vec3 const newPivot = glsl::vec3(newPoint, m_params.m_depth);

      ENormalType normalType1 = (step == 0) ? StartNormal : BaseNormal;
      ENormalType normalType2 = (step == steps - 1) ? EndNormal : BaseNormal;

      glsl::vec2 const leftNormal1 = GetNormal(segments[i], true /* isLeft */, normalType1);
      glsl::vec2 const rightNormal1 = GetNormal(segments[i], false /* isLeft */, normalType1);
      glsl::vec2 const leftNormal2 = GetNormal(segments[i], true /* isLeft */, normalType2);
      glsl::vec2 const rightNormal2 = GetNormal(segments[i], false /* isLeft */, normalType2);

      geometry.push_back(getLineVertex(segments[i], currentStartPivot, glsl::vec2(0, 0), offsetFromStart));
      geometry.push_back(getLineVertex(segments[i], currentStartPivot, leftNormal1, offsetFromStart));
      geometry.push_back(getLineVertex(segments[i], newPivot, glsl::vec2(0, 0), offsetFromStart));
      geometry.push_back(getLineVertex(segments[i], newPivot, leftNormal2, offsetFromStart));

      geometry.push_back(getLineVertex(segments[i], currentStartPivot, rightNormal1, offsetFromStart));
      geometry.push_back(getLineVertex(segments[i], currentStartPivot, glsl::vec2(0, 0), offsetFromStart));
      geometry.push_back(getLineVertex(segments[i], newPivot, rightNormal2, offsetFromStart));
      geometry.push_back(getLineVertex(segments[i], newPivot, glsl::vec2(0, 0), offsetFromStart));

      currentStartPivot = newPivot;
    }

    // generate joins
    if (i < segments.size() - 1)
    {
      glsl::vec2 n1 = segments[i].m_hasLeftJoin[EndPoint] ? segments[i].m_leftNormals[EndPoint] :
                                                            segments[i].m_rightNormals[EndPoint];
      glsl::vec2 n2 = segments[i + 1].m_hasLeftJoin[StartPoint] ? segments[i + 1].m_leftNormals[StartPoint] :
                                                                  segments[i + 1].m_rightNormals[StartPoint];

      float widthScalar = segments[i].m_hasLeftJoin[EndPoint] ? segments[i].m_rightWidthScalar[EndPoint].x :
                                                                segments[i].m_leftWidthScalar[EndPoint].x;

      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateJoinNormals(m_params.m_join, n1, n2, halfWidth, segments[i].m_hasLeftJoin[EndPoint],
                          widthScalar, normals);

      generateTriangles(glsl::vec3(segments[i].m_points[EndPoint], m_params.m_depth), normals);
    }

    // generate caps
    if (i == 0)
    {
      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(m_params.m_cap, segments[i].m_leftNormals[StartPoint],
                         segments[i].m_rightNormals[StartPoint], -segments[i].m_tangent,
                         halfWidth, true /* isStart */, normals);

      generateTriangles(glsl::vec3(segments[i].m_points[StartPoint], m_params.m_depth), normals);
    }

    if (i == segments.size() - 1)
    {
      vector<glsl::vec2> normals;
      normals.reserve(24);
      GenerateCapNormals(m_params.m_cap, segments[i].m_leftNormals[EndPoint],
                         segments[i].m_rightNormals[EndPoint], segments[i].m_tangent,
                         halfWidth, false /* isStart */, normals);

      generateTriangles(glsl::vec3(segments[i].m_points[EndPoint], m_params.m_depth), normals);
    }
  }

  dp::GLState state(gpu::LINE_PROGRAM, dp::GLState::GeometryLayer);
  state.SetColorTexture(colorRegion.GetTexture());
  state.SetMaskTexture(maskRegion.GetTexture());

  dp::AttributeProvider provider(1, geometry.size());
  provider.InitStream(0, gpu::LineVertex::GetBindingInfo(), make_ref(geometry.data()));
  batcher->InsertListOfStrip(state, make_ref(&provider), 4);

  dp::AttributeProvider joinsProvider(1, joinsGeometry.size());
  joinsProvider.InitStream(0, gpu::LineVertex::GetBindingInfo(), make_ref(joinsGeometry.data()));
  batcher->InsertTriangleList(state, make_ref(&joinsProvider));
}

} // namespace df

