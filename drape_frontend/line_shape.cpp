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

struct BaseBuilderParams
{
  dp::TextureManager::ColorRegion m_color;
  float m_pxHalfWidth;
  float m_depth;
  dp::LineCap m_cap;
  dp::LineJoin m_join;
};

template <typename TVertex>
class BaseLineBuilder : public ILineShapeInfo
{
public:
  BaseLineBuilder(BaseBuilderParams const & params, size_t geomsSize, size_t joinsSize)
    : m_params(params)
    , m_colorCoord(glsl::ToVec2(params.m_color.GetTexRect().Center()))
  {
    m_geometry.reserve(geomsSize);
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
    return m_params.m_pxHalfWidth;
  }

  dp::BindingInfo const & GetCapBindingInfo() override
  {
    return GetBindingInfo();
  }

  dp::GLState GetCapState() override
  {
    return GetState();
  }

  ref_ptr<void> GetCapData() override
  {
    return ref_ptr<void>();
  }

  size_t GetCapSize() override
  {
    return 0;
  }

protected:
  vector<glsl::vec2> const & GenerateCap(LineSegment const & segment, EPointType type,
                                         float sign, bool isStart)
  {
    m_normalBuffer.clear();
    m_normalBuffer.reserve(24);

    GenerateCapNormals(m_params.m_cap,
                       segment.m_leftNormals[type],
                       segment.m_rightNormals[type],
                       sign * segment.m_tangent,
                       GetHalfWidth(), isStart, m_normalBuffer);

    return m_normalBuffer;
  }

  vector<glsl::vec2> const & GenerateJoin(LineSegment const & seg1, LineSegment const & seg2)
  {
    m_normalBuffer.clear();
    m_normalBuffer.reserve(24);

    glsl::vec2 n1 = seg1.m_hasLeftJoin[EndPoint] ? seg1.m_leftNormals[EndPoint] :
                                                   seg1.m_rightNormals[EndPoint];

    glsl::vec2 n2 = seg2.m_hasLeftJoin[StartPoint] ? seg2.m_leftNormals[StartPoint] :
                                                     seg2.m_rightNormals[StartPoint];

    float widthScalar = seg1.m_hasLeftJoin[EndPoint] ? seg1.m_rightWidthScalar[EndPoint].x :
                                                       seg1.m_leftWidthScalar[EndPoint].x;

    GenerateJoinNormals(m_params.m_join, n1, n2, GetHalfWidth(),
                        seg1.m_hasLeftJoin[EndPoint], widthScalar, m_normalBuffer);

    return m_normalBuffer;
  }

protected:
  using V = TVertex;
  using TGeometryBuffer = vector<V>;

  TGeometryBuffer m_geometry;
  TGeometryBuffer m_joinGeom;

  vector<glsl::vec2> m_normalBuffer;

  BaseBuilderParams m_params;
  glsl::vec2 const m_colorCoord;
};

class SolidLineBuilder : public BaseLineBuilder<gpu::LineVertex>
{
  using TBase = BaseLineBuilder<gpu::LineVertex>;
  using TNormal = gpu::LineVertex::TNormal;

  struct CapVertex
  {
    using TPosition = gpu::LineVertex::TPosition;
    using TNormal = gpu::LineVertex::TNormal;
    using TTexCoord = gpu::LineVertex::TTexCoord;

    CapVertex() {}
    CapVertex(TPosition const & pos, TNormal const & normal, TTexCoord const & color)
      : m_position(pos)
      , m_normal(normal)
      , m_color(color)
    {
    }

    TPosition m_position;
    TNormal m_normal;
    TTexCoord m_color;
  };

  using TCapBuffer = vector<CapVertex>;

public:
  using BuilderParams = BaseBuilderParams;

  SolidLineBuilder(BuilderParams const & params, size_t pointsInSpline)
    : TBase(params, pointsInSpline * 2, (pointsInSpline - 2) * 8)
  {
  }

  dp::GLState GetState() override
  {
    dp::GLState state(gpu::LINE_PROGRAM, dp::GLState::GeometryLayer);
    state.SetColorTexture(m_params.m_color.GetTexture());
    return state;
  }

  dp::BindingInfo const & GetCapBindingInfo() override
  {
    if (m_params.m_cap != dp::RoundCap)
      return TBase::GetCapBindingInfo();

    static unique_ptr<dp::BindingInfo> s_capInfo;
    if (s_capInfo == nullptr)
    {
      dp::BindingFiller<CapVertex> filler(3);
      filler.FillDecl<CapVertex::TPosition>("a_position");
      filler.FillDecl<CapVertex::TNormal>("a_normal");
      filler.FillDecl<CapVertex::TTexCoord>("a_colorTexCoords");

      s_capInfo.reset(new dp::BindingInfo(filler.m_info));
    }

    return *s_capInfo;
  }

  dp::GLState GetCapState() override
  {
    if (m_params.m_cap != dp::RoundCap)
      return TBase::GetCapState();

    dp::GLState state(gpu::CAP_JOIN_PROGRAM, dp::GLState::GeometryLayer);
    state.SetColorTexture(m_params.m_color.GetTexture());
    state.SetDepthFunction(gl_const::GLLess);
    return state;
  }

  ref_ptr<void> GetCapData() override
  {
    return make_ref<void>(m_capGeometry.data());
  }

  size_t GetCapSize() override
  {
    return m_capGeometry.size();
  }

  void SubmitVertex(LineSegment const & segment, glsl::vec3 const & pivot,
                    glsl::vec2 const & normal, bool isLeft, float offsetFromStart)
  {
    UNUSED_VALUE(segment);
    UNUSED_VALUE(offsetFromStart);

    float halfWidth = GetHalfWidth();
    m_geometry.emplace_back(V(pivot, TNormal(halfWidth * normal, halfWidth * GetSide(isLeft)), m_colorCoord));
  }

  void SubmitJoin(LineSegment const & seg1, LineSegment const & seg2)
  {
    if (m_params.m_join == dp::RoundJoin)
      CreateRoundCap(seg1.m_points[EndPoint]);
    else
      SubmitJoinImpl(glsl::vec3(seg1.m_points[EndPoint], m_params.m_depth), GenerateJoin(seg1, seg2));
  }

  void SubmitCap(LineSegment const & segment, bool isStart)
  {
    EPointType const type = isStart ? StartPoint : EndPoint;
    if (m_params.m_cap != dp::RoundCap)
    {
      float const sign = isStart ? -1.0 : 1.0;
      SubmitJoinImpl(glsl::vec3(segment.m_points[type], m_params.m_depth), GenerateCap(segment, type, sign, isStart));
    }
    else
    {
      CreateRoundCap(segment.m_points[type]);
    }
  }

  float GetSide(bool isLeft)
  {
    return isLeft ? 1.0 : -1.0;
  }

private:
  void CreateRoundCap(glsl::vec2 const & pos)
  {
    m_capGeometry.reserve(8);

    float const radius = GetHalfWidth();

    m_capGeometry.push_back(CapVertex(CapVertex::TPosition(pos, m_params.m_depth),
                                      CapVertex::TNormal(-radius, radius, radius),
                                      CapVertex::TTexCoord(m_colorCoord)));
    m_capGeometry.push_back(CapVertex(CapVertex::TPosition(pos, m_params.m_depth),
                                      CapVertex::TNormal(-radius, -radius, radius),
                                      CapVertex::TTexCoord(m_colorCoord)));
    m_capGeometry.push_back(CapVertex(CapVertex::TPosition(pos, m_params.m_depth),
                                      CapVertex::TNormal(radius, radius, radius),
                                      CapVertex::TTexCoord(m_colorCoord)));
    m_capGeometry.push_back(CapVertex(CapVertex::TPosition(pos, m_params.m_depth),
                                      CapVertex::TNormal(radius, -radius, radius),
                                      CapVertex::TTexCoord(m_colorCoord)));
  }

  void SubmitJoinImpl(glsl::vec3 const & pivot, vector<glsl::vec2> const & normals)
  {
    float halfWidth = GetHalfWidth();
    size_t const trianglesCount = normals.size() / 3;
    for (int j = 0; j < trianglesCount; j++)
    {
      size_t baseIndex = 3 * j;
      m_joinGeom.push_back(V(pivot, TNormal(normals[baseIndex + 0], halfWidth), m_colorCoord));
      m_joinGeom.push_back(V(pivot, TNormal(normals[baseIndex + 1], halfWidth), m_colorCoord));
      m_joinGeom.push_back(V(pivot, TNormal(normals[baseIndex + 2], halfWidth), m_colorCoord));
    }
  }

private:
  TCapBuffer m_capGeometry;
};

class DashedLineBuilder : public BaseLineBuilder<gpu::DashedLineVertex>
{
  using TBase = BaseLineBuilder<gpu::DashedLineVertex>;

public:
  struct BuilderParams : BaseBuilderParams
  {
    dp::TextureManager::StippleRegion m_stipple;
    float m_glbHalfWidth;
    float m_baseGtoP;
  };

  DashedLineBuilder(BuilderParams const & params, size_t pointsInSpline)
    : TBase(params, pointsInSpline * 8, (pointsInSpline - 2) * 8)
    , m_texCoordGen(params.m_stipple, params.m_baseGtoP)
    , m_glbHalfWidth(params.m_glbHalfWidth)
    , m_baseGtoPScale(params.m_baseGtoP)
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
    state.SetColorTexture(m_params.m_color.GetTexture());
    state.SetMaskTexture(m_texCoordGen.GetRegion().GetTexture());
    return state;
  }

  void SubmitVertex(LineSegment const & segment, glsl::vec3 const & pivot,
                    glsl::vec2 const & normal, bool /*isLeft*/, float offsetFromStart)
  {
    float distance = GetProjectionLength(pivot.xy() + m_glbHalfWidth * normal,
                                         segment.m_points[StartPoint],
                                         segment.m_points[EndPoint]) - offsetFromStart;

    m_geometry.emplace_back(V(pivot, GetHalfWidth() * normal, m_colorCoord, m_texCoordGen.GetTexCoordsByDistance(distance)));
  }

  void SubmitJoin(LineSegment const & seg1, LineSegment const & seg2)
  {
    SubmitJoinImpl(glsl::vec3(seg1.m_points[EndPoint], m_params.m_depth), GenerateJoin(seg1, seg2));
  }

  void SubmitCap(LineSegment const & segment, bool isStart)
  {
    EPointType const type = isStart ? StartPoint : EndPoint;
    float const sign = isStart ? -1.0 : 1.0;
    SubmitJoinImpl(glsl::vec3(segment.m_points[type], m_params.m_depth), GenerateCap(segment, type, sign, isStart));
  }

private:
  void SubmitJoinImpl(glsl::vec3 const & pivot, vector<glsl::vec2> const & normals)
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

  auto commonParamsFiller = [&](BaseBuilderParams & p)
  {
    p.m_cap = m_params.m_cap;
    p.m_color = colorRegion;
    p.m_depth = m_params.m_depth;
    p.m_join = m_params.m_join;
    p.m_pxHalfWidth = pxHalfWidth;
  };

  if (m_params.m_pattern.empty())
  {
    SolidLineBuilder::BuilderParams p;
    commonParamsFiller(p);

    auto builder = make_unique<SolidLineBuilder>(p, m_spline->GetPath().size());
    Construct<SolidLineBuilder>(*builder);
    m_lineShapeInfo = move(builder);
  }
  else
  {
    dp::TextureManager::StippleRegion maskRegion;
    textures->GetStippleRegion(m_params.m_pattern, maskRegion);

    DashedLineBuilder::BuilderParams p;
    commonParamsFiller(p);
    p.m_stipple = maskRegion;
    p.m_baseGtoP = m_params.m_baseGtoPScale;
    p.m_glbHalfWidth = pxHalfWidth / m_params.m_baseGtoPScale;

    auto builder = make_unique<DashedLineBuilder>(p, m_spline->GetPath().size());
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
  float const joinsGenerationThreshold = 2.5f;
  bool generateJoins = true;
  if (builder.GetHalfWidth() <= joinsGenerationThreshold)
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
      builder.SubmitJoin(segments[i], segments[i + 1]);
  }

  builder.SubmitCap(segments[0], true /* isStart */);
  builder.SubmitCap(segments[segments.size() - 1], false /* isStart */);
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

  size_t capSize = m_lineShapeInfo->GetCapSize();
  if (capSize > 0)
  {
    dp::AttributeProvider capProvider(1, capSize);
    capProvider.InitStream(0, m_lineShapeInfo->GetCapBindingInfo(), m_lineShapeInfo->GetCapData());
    batcher->InsertListOfStrip(m_lineShapeInfo->GetCapState(), make_ref(&capProvider), dp::Batcher::VertexPerQuad);
  }
}

} // namespace df

