#include "drape_frontend/line_shape.hpp"

#include "drape_frontend/line_shape_helper.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/glsl_types.hpp"
#include "drape/glsl_func.hpp"
#include "drape/shader_def.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"

#include "base/logging.hpp"

namespace df
{

namespace
{

class TextureCoordGenerator
{
public:
  TextureCoordGenerator(dp::TextureManager::StippleRegion const & region, float const baseGtoPScale)
    : m_region(region)
    , m_baseGtoPScale(baseGtoPScale)
    , m_maskLength(static_cast<float>(m_region.GetMaskPixelLength()))
  {
  }

  float GetUvOffsetByDistance(float distance) const
  {
    return (distance * m_baseGtoPScale / m_maskLength);
  }

  glsl::vec2 GetTexCoordsByDistance(float distance) const
  {
    float normalizedOffset = min(GetUvOffsetByDistance(distance), 1.0f);
    return GetTexCoords(normalizedOffset);
  }

  glsl::vec2 GetTexCoords(float normalizedOffset) const
  {
    m2::RectF const & texRect = m_region.GetTexRect();
    return glsl::vec2(texRect.minX() + normalizedOffset * texRect.SizeX(), texRect.Center().y);
  }

  float GetMaskLength() const { return m_maskLength; }

  dp::TextureManager::StippleRegion const & GetRegion()
  {
    return m_region;
  }

private:
  dp::TextureManager::StippleRegion m_region;
  float const m_baseGtoPScale;
  float m_maskLength = 0.0f;
};

template <typename TVertex>
class BaseLineBuilder : public ILineShapeInfo
{
public:
  BaseLineBuilder(dp::TextureManager::ColorRegion const & color, float pxHalfWidth,
                  size_t geometrySize, size_t joinsSize)
    : m_color(color)
    , m_colorCoord(glsl::ToVec2(m_color.GetTexRect().Center()))
    , m_pxHalfWidth(pxHalfWidth)
  {
    m_geometry.reserve(geometrySize);
    m_joinGeom.reserve(joinsSize);
  }

  void GetTexturingInfo(float const globalLength, int & steps, float & maskSize)
  {
    UNUSED_VALUE(globalLength);
    UNUSED_VALUE(steps);
    UNUSED_VALUE(maskSize);
  }

  dp::BindingInfo const & GetBindingInfo() override
  {
    return TVertex::GetBindingInfo();
  }

  ref_ptr<void> GetLineData() override
  {
    return make_ref(m_geometry.data());
  }

  size_t GetLineSize() override
  {
    return m_geometry.size();
  }

  ref_ptr<void> GetJoinData() override
  {
    return make_ref(m_joinGeom.data());
  }

  size_t GetJoinSize() override
  {
    return m_joinGeom.size();
  }

  float GetHalfWidth()
  {
    return m_pxHalfWidth;
  }

protected:
  using V = TVertex;
  using TGeometryBuffer = vector<V>;

  TGeometryBuffer m_geometry;
  TGeometryBuffer m_joinGeom;

  dp::TextureManager::ColorRegion m_color;
  glsl::vec2 const m_colorCoord;
  float const m_pxHalfWidth;
};

class SolidLineBuilder : public BaseLineBuilder<gpu::LineVertex>
{
  using TBase = BaseLineBuilder<gpu::LineVertex>;
  using TNormal = gpu::LineVertex::TNormal;

public:
  SolidLineBuilder(dp::TextureManager::ColorRegion const & color, float const  pxHalfWidth, size_t pointsInSpline)
    : TBase(color, pxHalfWidth, pointsInSpline * 2, (pointsInSpline - 2) * 8)
  {
  }

  dp::GLState GetState() override
  {
    dp::GLState state(gpu::LINE_PROGRAM, dp::GLState::GeometryLayer);
    state.SetColorTexture(m_color.GetTexture());
    return state;
  }

  void SubmitVertex(LineSegment const & segment, glsl::vec3 const & pivot,
                    glsl::vec2 const & normal, bool isLeft, float offsetFromStart)
  {
    UNUSED_VALUE(segment);
    UNUSED_VALUE(offsetFromStart);

    m_geometry.emplace_back(V(pivot, TNormal(m_pxHalfWidth * normal, m_pxHalfWidth * GetSide(isLeft)), m_colorCoord));
  }

  void SubmitJoin(glsl::vec3 const & pivot, vector<glsl::vec2> const & normals)
  {
    size_t const trianglesCount = normals.size() / 3;
    for (int j = 0; j < trianglesCount; j++)
    {
      size_t baseIndex = 3 * j;
      m_joinGeom.push_back(V(pivot, TNormal(normals[baseIndex + 0], m_pxHalfWidth), m_colorCoord));
      m_joinGeom.push_back(V(pivot, TNormal(normals[baseIndex + 1], m_pxHalfWidth), m_colorCoord));
      m_joinGeom.push_back(V(pivot, TNormal(normals[baseIndex + 2], m_pxHalfWidth), m_colorCoord));
    }
  }

  float GetSide(bool isLeft)
  {
    return isLeft ? 1.0 : -1.0;
  }
};

class DashedLineBuilder : public BaseLineBuilder<gpu::DashedLineVertex>
{
  using TBase = BaseLineBuilder<gpu::DashedLineVertex>;

public:
  DashedLineBuilder(dp::TextureManager::ColorRegion const & color,
                    dp::TextureManager::StippleRegion const & stipple,
                    float glbHalfWidth, float pxHalfWidth, float baseGtoP,
                    size_t pointsInSpline)
    : TBase(color, pxHalfWidth, pointsInSpline * 8, (pointsInSpline - 2) * 8)
    , m_texCoordGen(stipple, baseGtoP)
    , m_glbHalfWidth(glbHalfWidth)
    , m_baseGtoPScale(baseGtoP)
  {
  }

  void GetTexturingInfo(float const globalLength, int & steps, float & maskSize)
  {
    float const pixelLen = globalLength * m_baseGtoPScale;
    steps = static_cast<int>((pixelLen + m_texCoordGen.GetMaskLength() - 1) / m_texCoordGen.GetMaskLength());
    maskSize = globalLength / steps;
  }

  dp::GLState GetState() override
  {
    dp::GLState state(gpu::DASHED_LINE_PROGRAM, dp::GLState::GeometryLayer);
    state.SetColorTexture(m_color.GetTexture());
    state.SetMaskTexture(m_texCoordGen.GetRegion().GetTexture());
    return state;
  }

  void SubmitVertex(LineSegment const & segment, glsl::vec3 const & pivot,
                    glsl::vec2 const & normal, bool /*isLeft*/, float offsetFromStart)
  {
    float distance = GetProjectionLength(pivot.xy() + m_glbHalfWidth * normal,
                                         segment.m_points[StartPoint],
                                         segment.m_points[EndPoint]) - offsetFromStart;

    m_geometry.emplace_back(V(pivot, m_pxHalfWidth * normal, m_colorCoord, m_texCoordGen.GetTexCoordsByDistance(distance)));
  }

  void SubmitJoin(glsl::vec3 const & pivot, vector<glsl::vec2> const & normals)
  {
    size_t const trianglesCount = normals.size() / 3;
    for (int j = 0; j < trianglesCount; j++)
    {
      size_t baseIndex = 3 * j;
      glsl::vec2 texCoord = m_texCoordGen.GetTexCoords(0.0f);
      m_joinGeom.push_back(V(pivot, normals[baseIndex + 0], m_colorCoord, texCoord));
      m_joinGeom.push_back(V(pivot, normals[baseIndex + 1], m_colorCoord, texCoord));
      m_joinGeom.push_back(V(pivot, normals[baseIndex + 2], m_colorCoord, texCoord));
    }
  }

private:
  TextureCoordGenerator m_texCoordGen;
  float const m_glbHalfWidth;
  float const m_baseGtoPScale;
};

} // namespace

LineShape::LineShape(m2::SharedSpline const & spline, LineViewParams const & params)
  : m_params(params)
  , m_spline(spline)
{
  ASSERT_GREATER(m_spline->GetPath().size(), 1, ());
}

void LineShape::Prepare(ref_ptr<dp::TextureManager> textures) const
{
  dp::TextureManager::ColorRegion colorRegion;
  textures->GetColorRegion(m_params.m_color, colorRegion);
  float const pxHalfWidth = m_params.m_width / 2.0f;

  if (m_params.m_pattern.empty())
  {
    auto builder = make_unique<SolidLineBuilder>(colorRegion, pxHalfWidth, m_spline->GetPath().size());
    Construct<SolidLineBuilder>(*builder);
    m_lineShapeInfo = move(builder);
  }
  else
  {
    dp::TextureManager::StippleRegion maskRegion;
    textures->GetStippleRegion(m_params.m_pattern, maskRegion);

    float const glbHalfWidth = pxHalfWidth / m_params.m_baseGtoPScale;

    auto builder = make_unique<DashedLineBuilder>(colorRegion, maskRegion, glbHalfWidth,
                                                  pxHalfWidth, m_params.m_baseGtoPScale,
                                                  m_spline->GetPath().size());
    Construct<DashedLineBuilder>(*builder);
    m_lineShapeInfo = move(builder);
  }
}

template <typename TBuilder>
void LineShape::Construct(TBuilder & builder) const
{
  vector<m2::PointD> const & path = m_spline->GetPath();
  ASSERT(path.size() > 1, ());

  // skip joins generation
  float const halfWidth = builder.GetHalfWidth();
  float const joinsGenerationThreshold = 2.5f;
  bool generateJoins = true;
  if (halfWidth <= joinsGenerationThreshold)
    generateJoins = false;

  // constuct segments
  vector<LineSegment> segments;
  segments.reserve(path.size() - 1);
  ConstructLineSegments(path, segments);

  // build geometry
  for (size_t i = 0; i < segments.size(); i++)
  {
    if (generateJoins)
    {
      UpdateNormals(&segments[i], (i > 0) ? &segments[i - 1] : nullptr,
                    (i < segments.size() - 1) ? &segments[i + 1] : nullptr);
    }

    // calculate number of steps to cover line segment
    float const initialGlobalLength = glsl::length(segments[i].m_points[EndPoint] - segments[i].m_points[StartPoint]);
    int steps = 1;
    float maskSize = initialGlobalLength;
    builder.GetTexturingInfo(initialGlobalLength, steps, maskSize);

    // generate main geometry
    float currentSize = 0;
    glsl::vec3 currentStartPivot = glsl::vec3(segments[i].m_points[StartPoint], m_params.m_depth);
    for (int step = 0; step < steps; step++)
    {
      float const offsetFromStart = currentSize;
      currentSize += maskSize;

      glsl::vec2 const newPoint = segments[i].m_points[StartPoint] + segments[i].m_tangent * currentSize;
      glsl::vec3 const newPivot = glsl::vec3(newPoint, m_params.m_depth);
      glsl::vec2 const leftNormal = GetNormal(segments[i], true /* isLeft */, BaseNormal);
      glsl::vec2 const rightNormal = GetNormal(segments[i], false /* isLeft */, BaseNormal);

      builder.SubmitVertex(segments[i], currentStartPivot, rightNormal, false /* isLeft */, offsetFromStart);
      builder.SubmitVertex(segments[i], currentStartPivot, leftNormal, true /* isLeft */, offsetFromStart);
      builder.SubmitVertex(segments[i], newPivot, rightNormal, false /* isLeft */, offsetFromStart);
      builder.SubmitVertex(segments[i], newPivot, leftNormal, true /* isLeft */, offsetFromStart);

      currentStartPivot = newPivot;
    }

    // generate joins
    if (generateJoins && i < segments.size() - 1)
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

      builder.SubmitJoin(glsl::vec3(segments[i].m_points[EndPoint], m_params.m_depth), normals);
    }
  }

  vector<glsl::vec2> normals;
  normals.reserve(24);

  {
    LineSegment const & segment = segments[0];
    GenerateCapNormals(m_params.m_cap,
                       segment.m_leftNormals[StartPoint],
                       segment.m_rightNormals[StartPoint],
                       -segment.m_tangent,
                       halfWidth, true /* isStart */, normals);

    builder.SubmitJoin(glsl::vec3(segment.m_points[StartPoint], m_params.m_depth), normals);
    normals.clear();
  }

  {
    size_t index = segments.size() - 1;
    LineSegment const & segment = segments[index];
    GenerateCapNormals(m_params.m_cap,
                       segment.m_leftNormals[EndPoint],
                       segment.m_rightNormals[EndPoint],
                       segment.m_tangent,
                       halfWidth, false /* isStart */, normals);

    builder.SubmitJoin(glsl::vec3(segment.m_points[EndPoint], m_params.m_depth), normals);
  }
}

void LineShape::Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const
{
  if (!m_lineShapeInfo)
    Prepare(textures);

  ASSERT(m_lineShapeInfo != nullptr, ());
  dp::GLState state = m_lineShapeInfo->GetState();

  dp::AttributeProvider provider(1, m_lineShapeInfo->GetLineSize());
  provider.InitStream(0, m_lineShapeInfo->GetBindingInfo(), m_lineShapeInfo->GetLineData());
  batcher->InsertListOfStrip(state, make_ref(&provider), dp::Batcher::VertexPerQuad);

  size_t joinSize = m_lineShapeInfo->GetJoinSize();
  if (joinSize > 0)
  {
    dp::AttributeProvider joinsProvider(1, joinSize);
    joinsProvider.InitStream(0, m_lineShapeInfo->GetBindingInfo(), m_lineShapeInfo->GetJoinData());
    batcher->InsertTriangleList(state, make_ref(&joinsProvider));
  }
}

} // namespace df

