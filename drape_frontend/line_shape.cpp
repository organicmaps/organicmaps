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
  TextureCoordGenerator(dp::TextureManager::StippleRegion const & region)
    : m_region(region)
    , m_maskLength(static_cast<float>(m_region.GetMaskPixelLength()))
  {}

  glsl::vec4 GetTexCoordsByDistance(float distance) const
  {
    return GetTexCoords(distance / m_maskLength);
  }

  glsl::vec4 GetTexCoords(float offset) const
  {
    m2::RectF const & texRect = m_region.GetTexRect();
    return glsl::vec4(offset, texRect.minX(), texRect.SizeX(), texRect.Center().y);
  }

  float GetMaskLength() const
  {
    return m_maskLength;
  }

  dp::TextureManager::StippleRegion const & GetRegion() const
  {
    return m_region;
  }

private:
  dp::TextureManager::StippleRegion const m_region;
  float const m_maskLength;
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

  float GetSide(bool isLeft) const
  {
    return isLeft ? 1.0 : -1.0;
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
  {}

  dp::GLState GetState() override
  {
    dp::GLState state(gpu::LINE_PROGRAM, dp::GLState::GeometryLayer);
    state.SetColorTexture(m_params.m_color.GetTexture());
    return state;
  }

  dp::BindingInfo const & GetCapBindingInfo() override
  {
    if (m_params.m_cap == dp::ButtCap)
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
    if (m_params.m_cap == dp::ButtCap)
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

  void SubmitVertex(glsl::vec3 const & pivot, glsl::vec2 const & normal, bool isLeft)
  {
    float const halfWidth = GetHalfWidth();
    m_geometry.emplace_back(V(pivot, TNormal(halfWidth * normal, halfWidth * GetSide(isLeft)), m_colorCoord));
  }

  void SubmitJoin(glsl::vec2 const & pos)
  {
    if (m_params.m_join == dp::RoundJoin)
      CreateRoundCap(pos);
  }

  void SubmitCap(glsl::vec2 const & pos)
  {
    if (m_params.m_cap != dp::ButtCap)
      CreateRoundCap(pos);
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

private:
  TCapBuffer m_capGeometry;
};

class DashedLineBuilder : public BaseLineBuilder<gpu::DashedLineVertex>
{
  using TBase = BaseLineBuilder<gpu::DashedLineVertex>;
  using TNormal = gpu::LineVertex::TNormal;

public:
  struct BuilderParams : BaseBuilderParams
  {
    dp::TextureManager::StippleRegion m_stipple;
    float m_glbHalfWidth;
    float m_baseGtoP;
  };

  DashedLineBuilder(BuilderParams const & params, size_t pointsInSpline)
    : TBase(params, pointsInSpline * 8, (pointsInSpline - 2) * 8)
    , m_texCoordGen(params.m_stipple)
    , m_baseGtoPScale(params.m_baseGtoP)
  {}

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

  void SubmitVertex(glsl::vec3 const & pivot, glsl::vec2 const & normal, bool isLeft, float offsetFromStart)
  {
    float const halfWidth = GetHalfWidth();
    m_geometry.emplace_back(V(pivot, TNormal(halfWidth * normal, halfWidth * GetSide(isLeft)),
                              m_colorCoord, m_texCoordGen.GetTexCoordsByDistance(offsetFromStart)));
  }

private:
  TextureCoordGenerator m_texCoordGen;
  float const m_baseGtoPScale;
};

} // namespace

LineShape::LineShape(m2::SharedSpline const & spline, LineViewParams const & params)
  : m_params(params)
  , m_spline(spline)
{
  ASSERT_GREATER(m_spline->GetPath().size(), 1, ());
}

template <typename TBuilder>
void LineShape::Construct(TBuilder & builder) const
{
  ASSERT(false, ("No implementation"));
}

// Specialization optimized for dashed lines.
template <>
void LineShape::Construct<DashedLineBuilder>(DashedLineBuilder & builder) const
{
  vector<m2::PointD> const & path = m_spline->GetPath();
  ASSERT_GREATER(path.size(), 1, ());

  // build geometry
  for (size_t i = 1; i < path.size(); ++i)
  {
    if (path[i].EqualDxDy(path[i - 1], 1.0E-5))
      continue;

    glsl::vec2 const p1 = glsl::vec2(path[i - 1].x, path[i - 1].y);
    glsl::vec2 const p2 = glsl::vec2(path[i].x, path[i].y);
    glsl::vec2 tangent, leftNormal, rightNormal;
    CalculateTangentAndNormals(p1, p2, tangent, leftNormal, rightNormal);

    // calculate number of steps to cover line segment
    float const initialGlobalLength = glsl::length(p2 - p1);
    int steps = 1;
    float maskSize = initialGlobalLength;
    builder.GetTexturingInfo(initialGlobalLength, steps, maskSize);

    // generate vertices
    float currentSize = 0;
    glsl::vec3 currentStartPivot = glsl::vec3(p1, m_params.m_depth);
    for (int step = 0; step < steps; step++)
    {
      currentSize += maskSize;
      glsl::vec3 const newPivot = glsl::vec3(p1 + tangent * currentSize, m_params.m_depth);

      builder.SubmitVertex(currentStartPivot, rightNormal, false /* isLeft */, 0.0);
      builder.SubmitVertex(currentStartPivot, leftNormal, true /* isLeft */, 0.0);
      builder.SubmitVertex(newPivot, rightNormal, false /* isLeft */, maskSize);
      builder.SubmitVertex(newPivot, leftNormal, true /* isLeft */, maskSize);

      currentStartPivot = newPivot;
    }
  }
}

// Specialization optimized for solid lines.
template <>
void LineShape::Construct<SolidLineBuilder>(SolidLineBuilder & builder) const
{
  vector<m2::PointD> const & path = m_spline->GetPath();
  ASSERT_GREATER(path.size(), 1, ());

  // skip joins generation
  float const kJoinsGenerationThreshold = 2.5f;
  bool generateJoins = true;
  if (builder.GetHalfWidth() <= kJoinsGenerationThreshold)
    generateJoins = false;

  // build geometry
  glsl::vec2 firstPoint = glsl::vec2(path.front().x, path.front().y);
  glsl::vec2 lastPoint;
  bool hasConstructedSegments = false;
  for (size_t i = 1; i < path.size(); ++i)
  {
    if (path[i].EqualDxDy(path[i - 1], 1.0E-5))
      continue;

    glsl::vec2 const p1 = glsl::vec2(path[i - 1].x, path[i - 1].y);
    glsl::vec2 const p2 = glsl::vec2(path[i].x, path[i].y);
    glsl::vec2 tangent, leftNormal, rightNormal;
    CalculateTangentAndNormals(p1, p2, tangent, leftNormal, rightNormal);

    glsl::vec3 const startPoint = glsl::vec3(p1, m_params.m_depth);
    glsl::vec3 const endPoint = glsl::vec3(p2, m_params.m_depth);

    builder.SubmitVertex(startPoint, rightNormal, false /* isLeft */);
    builder.SubmitVertex(startPoint, leftNormal, true /* isLeft */);
    builder.SubmitVertex(endPoint, rightNormal, false /* isLeft */);
    builder.SubmitVertex(endPoint, leftNormal, true /* isLeft */);

    // generate joins
    if (generateJoins && i < path.size() - 1)
      builder.SubmitJoin(p2);

    lastPoint = p2;
    hasConstructedSegments = true;
  }

  if (hasConstructedSegments)
  {
    builder.SubmitCap(firstPoint);
    builder.SubmitCap(lastPoint);
  }
}

void LineShape::Prepare(ref_ptr<dp::TextureManager> textures) const
{
  dp::TextureManager::ColorRegion colorRegion;
  textures->GetColorRegion(m_params.m_color, colorRegion);
  float const pxHalfWidth = m_params.m_width / 2.0f;

  auto commonParamsBuilder = [&](BaseBuilderParams & p)
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
    commonParamsBuilder(p);

    auto builder = make_unique<SolidLineBuilder>(p, m_spline->GetPath().size());
    Construct<SolidLineBuilder>(*builder);
    m_lineShapeInfo = move(builder);
  }
  else
  {
    dp::TextureManager::StippleRegion maskRegion;
    textures->GetStippleRegion(m_params.m_pattern, maskRegion);

    DashedLineBuilder::BuilderParams p;
    commonParamsBuilder(p);
    p.m_stipple = maskRegion;
    p.m_baseGtoP = m_params.m_baseGtoPScale;
    p.m_glbHalfWidth = pxHalfWidth / m_params.m_baseGtoPScale;

    auto builder = make_unique<DashedLineBuilder>(p, m_spline->GetPath().size());
    Construct<DashedLineBuilder>(*builder);
    m_lineShapeInfo = move(builder);
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

  size_t capSize = m_lineShapeInfo->GetCapSize();
  if (capSize > 0)
  {
    dp::AttributeProvider capProvider(1, capSize);
    capProvider.InitStream(0, m_lineShapeInfo->GetCapBindingInfo(), m_lineShapeInfo->GetCapData());
    batcher->InsertListOfStrip(m_lineShapeInfo->GetCapState(), make_ref(&capProvider), dp::Batcher::VertexPerQuad);
  }
}

} // namespace df

