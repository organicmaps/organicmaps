#include "drape_frontend/line_shape.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/glsl_types.hpp"
#include "drape/support_manager.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "indexer/scales.hpp"

#include <algorithm>
#include <vector>

namespace df
{
namespace
{
class TextureCoordGenerator
{
public:
  explicit TextureCoordGenerator(dp::TextureManager::StippleRegion const & region)
    : m_region(region)
    , m_maskSize(m_region.GetMaskPixelSize())
  {}

  glsl::vec4 GetTexCoordsByDistance(float distance, bool isLeft) const
  {
    m2::RectF const & texRect = m_region.GetTexRect();
    return {distance / GetMaskLength(), texRect.minX(), texRect.SizeX(), isLeft ? texRect.minY() : texRect.maxY()};
  }

  uint32_t GetMaskLength() const { return m_maskSize.x; }

  dp::TextureManager::StippleRegion const & GetRegion() const { return m_region; }

private:
  dp::TextureManager::StippleRegion const m_region;
  m2::PointU const m_maskSize;
};

struct BaseBuilderParams
{
  dp::TextureManager::ColorRegion m_color;
  float m_pxHalfWidth;
  float m_depth;
  bool m_depthTestEnabled;
  DepthLayer m_depthLayer;
  dp::LineCap m_cap;
  dp::LineJoin m_join;
};

template <typename TVertex>
class BaseLineBuilder : public LineShapeInfo
{
public:
  BaseLineBuilder(BaseBuilderParams const & params, size_t geomsSize, size_t joinsSize)
    : m_params(params)
    , m_colorCoord(glsl::ToVec2(params.m_color.GetTexRect().Center()))
  {
    m_geometry.reserve(geomsSize);
    // m_joinGeom.reserve(joinsSize);
  }

  dp::BindingInfo const & GetBindingInfo() override { return TVertex::GetBindingInfo(); }

  ref_ptr<void> GetLineData() override { return make_ref(m_geometry.data()); }

  uint32_t GetLineSize() override { return static_cast<uint32_t>(m_geometry.size()); }

  //  ref_ptr<void> GetJoinData() override
  //  {
  //    return make_ref(m_joinGeom.data());
  //  }

  //  uint32_t GetJoinSize() override
  //  {
  //    return static_cast<uint32_t>(m_joinGeom.size());
  //  }

  float GetHalfWidth() { return m_params.m_pxHalfWidth; }

  dp::BindingInfo const & GetCapBindingInfo() override { return GetBindingInfo(); }

  dp::RenderState GetCapState() override { return GetState(); }

  ref_ptr<void> GetCapData() override { return ref_ptr<void>(); }

  uint32_t GetCapSize() override { return 0; }

  float GetSide(bool isLeft) const { return isLeft ? 1.0f : -1.0f; }

protected:
  using V = TVertex;
  using TGeometryBuffer = gpu::VBReservedSizeT<V>;

  TGeometryBuffer m_geometry;
  // TGeometryBuffer m_joinGeom;

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
    {}

    TPosition m_position;
    TNormal m_normal;
    TTexCoord m_color;
  };

  using TCapBuffer = gpu::VBUnknownSizeT<CapVertex>;

public:
  using BuilderParams = BaseBuilderParams;

  SolidLineBuilder(BuilderParams const & params, size_t pointsInSpline)
    : TBase(params, pointsInSpline * 2, (pointsInSpline - 2) * 8)
  {}

  dp::RenderState GetState() override
  {
    auto state = CreateRenderState(gpu::Program::Line, m_params.m_depthLayer);
    state.SetColorTexture(m_params.m_color.GetTexture());
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    return state;
  }

  dp::BindingInfo const & GetCapBindingInfo() override
  {
    ASSERT(!m_capGeometry.empty(), ());

    static std::unique_ptr<dp::BindingInfo> s_capInfo;
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

  dp::RenderState GetCapState() override
  {
    ASSERT(!m_capGeometry.empty(), ());

    auto state = CreateRenderState(gpu::Program::CapJoin, m_params.m_depthLayer);
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    state.SetColorTexture(m_params.m_color.GetTexture());
    state.SetDepthFunction(dp::TestFunction::Less);
    return state;
  }

  ref_ptr<void> GetCapData() override { return make_ref<void>(m_capGeometry.data()); }

  uint32_t GetCapSize() override { return static_cast<uint32_t>(m_capGeometry.size()); }

  void SubmitVertex(glsl::vec3 const & pivot, glsl::vec2 const & normal, bool isLeft)
  {
    float const halfWidth = GetHalfWidth();
    m_geometry.emplace_back(pivot, TNormal(halfWidth * normal, halfWidth * GetSide(isLeft)), m_colorCoord);
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
    // Here we use an equilateral triangle to render circle (incircle of a triangle).
    static float const kSqrt3 = sqrt(3.0f);
    float const radius = GetHalfWidth();

    m_capGeometry.emplace_back(CapVertex::TPosition(pos, m_params.m_depth),
                               CapVertex::TNormal(-radius * kSqrt3, -radius, radius),
                               CapVertex::TTexCoord(m_colorCoord));
    m_capGeometry.emplace_back(CapVertex::TPosition(pos, m_params.m_depth),
                               CapVertex::TNormal(radius * kSqrt3, -radius, radius),
                               CapVertex::TTexCoord(m_colorCoord));
    m_capGeometry.emplace_back(CapVertex::TPosition(pos, m_params.m_depth),
                               CapVertex::TNormal(0, 2.0f * radius, radius), CapVertex::TTexCoord(m_colorCoord));
  }

private:
  TCapBuffer m_capGeometry;
};

class SimpleSolidLineBuilder : public BaseLineBuilder<gpu::AreaVertex>
{
  using TBase = BaseLineBuilder<gpu::AreaVertex>;

public:
  using BuilderParams = BaseBuilderParams;

  SimpleSolidLineBuilder(BuilderParams const & params, size_t pointsInSpline, int lineWidth)
    : TBase(params, pointsInSpline, 0)
    , m_lineWidth(lineWidth)
  {}

  dp::RenderState GetState() override
  {
    auto state = CreateRenderState(gpu::Program::AreaOutline, m_params.m_depthLayer);
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    state.SetColorTexture(m_params.m_color.GetTexture());
    state.SetDrawAsLine(true);
    state.SetLineWidth(m_lineWidth);
    return state;
  }

  void SubmitVertex(glsl::vec3 const & pivot) { m_geometry.emplace_back(pivot, m_colorCoord); }

private:
  int m_lineWidth;
};

class DashedLineBuilder : public BaseLineBuilder<gpu::DashedLineVertex>
{
  using TBase = BaseLineBuilder<gpu::DashedLineVertex>;
  using TNormal = gpu::LineVertex::TNormal;

public:
  struct BuilderParams : BaseBuilderParams
  {
    dp::TextureManager::StippleRegion m_stipple;
    float m_baseGtoP;
  };

  DashedLineBuilder(BuilderParams const & params, size_t pointsInSpline)
    : TBase(params, pointsInSpline * 8, (pointsInSpline - 2) * 8)
    , m_texCoordGen(params.m_stipple)
    , m_baseGtoPScale(params.m_baseGtoP)
  {}

  float GetMaskLengthG() const { return m_texCoordGen.GetMaskLength() / m_baseGtoPScale; }

  dp::RenderState GetState() override
  {
    auto state = CreateRenderState(gpu::Program::DashedLine, m_params.m_depthLayer);
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    state.SetColorTexture(m_params.m_color.GetTexture());
    state.SetMaskTexture(m_texCoordGen.GetRegion().GetTexture());
    return state;
  }

  void SubmitVertex(glsl::vec3 const & pivot, glsl::vec2 const & normal, bool isLeft, float offsetFromStart)
  {
    float const halfWidth = GetHalfWidth();
    m_geometry.emplace_back(pivot, TNormal(halfWidth * normal, halfWidth * GetSide(isLeft)), m_colorCoord,
                            m_texCoordGen.GetTexCoordsByDistance(offsetFromStart, isLeft));
  }

private:
  TextureCoordGenerator m_texCoordGen;
  float const m_baseGtoPScale;
};

glsl::vec2 ToShapeVertex2(m2::PointD const & vertex, m2::PointD const & tileCenter)
{
  return glsl::ToVec2(MapShape::ConvertToLocal(vertex, tileCenter, kShapeCoordScalar));
}

template <class FnT>
void ForEachSplineSection(m2::SharedSpline const & spline, m2::PointD const & tileCenter, FnT && fn)
{
  std::vector<m2::PointD> const & path = spline->GetPath();
  ASSERT(!path.empty(), ());
  size_t const sz = path.size() - 1;

  for (size_t i = 0, j = 1; j <= sz; ++j)
  {
    /// @todo Make this kind of filtration in Spline?
    if (path[i].EqualDxDy(path[j], kMwmPointAccuracy) && j < sz)
      continue;

    std::pair<m2::PointD, double> tanlen;
    if (j == i + 1)
    {
      // Fast path - take calculated tangent and length.
      tanlen = spline->GetTangentAndLength(i);
    }
    else
    {
      tanlen.first = path[j] - path[i];
      tanlen.second = tanlen.first.Length();
      tanlen.first = tanlen.first / tanlen.second;
    }

    glsl::vec2 const tangent = glsl::ToVec2(tanlen.first);

    fn(ToShapeVertex2(path[i], tileCenter), ToShapeVertex2(path[j], tileCenter), tangent, tanlen.second,
       {-tangent.y, tangent.x}, {tangent.y, -tangent.x}, (i == 0 ? 0x1 : 0) + (j == sz ? 0x2 : 0));

    i = j;
  }
}

template <class TBuilder>
void ConstructSolidLine(m2::SharedSpline const & spline, m2::PointD const & tileCenter, float depth, TBuilder & builder)
{
  bool const generateJoins = builder.GetHalfWidth() > 2.5f;

  ForEachSplineSection(spline, tileCenter,
                       [&](glsl::vec2 const & p1, glsl::vec2 const & p2, glsl::vec2 const & tangent, double,
                           glsl::vec2 const & leftNormal, glsl::vec2 const & rightNormal, int flag)
  {
    builder.SubmitVertex({p1, depth}, rightNormal, false /* isLeft */);
    builder.SubmitVertex({p1, depth}, leftNormal, true /* isLeft */);
    builder.SubmitVertex({p2, depth}, rightNormal, false /* isLeft */);
    builder.SubmitVertex({p2, depth}, leftNormal, true /* isLeft */);

    if (flag & 0x1)
      builder.SubmitCap(p1);
    if (flag & 0x2)
      builder.SubmitCap(p2);
    else if (generateJoins)
      builder.SubmitJoin(p2);
  });
}

template <class TBuilder>
void ConstructDashedLine(m2::SharedSpline const & spline, m2::PointD const & tileCenter, float depth,
                         TBuilder & builder)
{
  float constexpr toShapeFactor = kShapeCoordScalar;  // the same as in ToShapeVertex2

  // Each segment should lie in pattern mask according to the "longest" possible pixel length in current tile.
  // Since, we calculate vertices once, usually for the "smallest" tile scale, need to apply divide factor here.
  // In other words, if m_baseGtoPScale = Scale(tileLevel), we should use Scale(tileLevel + 1) to calculate
  // 'maskLengthG'.
  /// @todo Logically, the factor should be 2, but drawing artifacts are still present at higher visual scales.
  /// Use 3 for the best quality, but need to review here, probably I missed something.
  float const maskLengthG = builder.GetMaskLengthG() / 3;

  float offset = 0;
  ForEachSplineSection(spline, tileCenter,
                       [&](glsl::vec2 const & p1, glsl::vec2 const & p2, glsl::vec2 const & tangent, float toDraw,
                           glsl::vec2 const & leftNormal, glsl::vec2 const & rightNormal, int)
  {
    glsl::vec2 currPivot = p1;
    do
    {
      glsl::vec2 nextPivot;
      float nextOffset = offset + toDraw;
      if (maskLengthG >= nextOffset)
      {
        // Fast lane, where most of segments, that fit into mask, should draw.
        nextPivot = p2;
        toDraw = 0;
      }
      else
      {
        // Break path section here.
        float const len = maskLengthG - offset;
        ASSERT_GREATER(len, 0, ());
        nextPivot = currPivot + tangent * (len * toShapeFactor);

        nextOffset = maskLengthG;
        toDraw -= len;
      }

      builder.SubmitVertex({currPivot, depth}, rightNormal, false /* isLeft */, offset);
      builder.SubmitVertex({currPivot, depth}, leftNormal, true /* isLeft */, offset);
      builder.SubmitVertex({nextPivot, depth}, rightNormal, false /* isLeft */, nextOffset);
      builder.SubmitVertex({nextPivot, depth}, leftNormal, true /* isLeft */, nextOffset);

      currPivot = nextPivot;
      offset = nextOffset;
      if (offset >= maskLengthG)
        offset = 0;
    }
    while (toDraw > 0);
  });
}

void ConstructSimpleSolidLine(m2::SharedSpline const & spline, m2::PointD const & tileCenter, float depth,
                              SimpleSolidLineBuilder & builder)
{
  std::vector<m2::PointD> const & path = spline->GetPath();
  ASSERT_GREATER(path.size(), 1, ());

  for (m2::PointD const & pt : path)
    builder.SubmitVertex(glsl::vec3(ToShapeVertex2(pt, tileCenter), depth));
}
}  // namespace

LineShape::LineShape(m2::SharedSpline const & spline, LineViewParams const & params)
  : m_params(params)
  , m_spline(spline)
  , m_isSimple(false)
{
  ASSERT_GREATER(m_spline->GetPath().size(), 1, ());
}

bool LineShape::CanBeSimplified(int & lineWidth) const
{
  // Disable simplification for world map.
  if (m_params.m_zoomLevel > 0 && m_params.m_zoomLevel <= scales::GetUpperCountryScale())
    return false;

  float const width = std::min(2.5f, dp::SupportManager::Instance().GetMaxLineWidth());
  if (m_params.m_width <= width)
  {
    lineWidth = std::max(1, static_cast<int>(m_params.m_width));
    return true;
  }

  lineWidth = 1;
  return false;
}

void LineShape::Prepare(ref_ptr<dp::TextureManager> textures) const
{
  auto commonParamsBuilder = [this, textures](BaseBuilderParams & p)
  {
    dp::TextureManager::ColorRegion colorRegion;
    textures->GetColorRegion(m_params.m_color, colorRegion);

    p.m_cap = m_params.m_cap;
    p.m_color = colorRegion;
    p.m_depthTestEnabled = m_params.m_depthTestEnabled;
    p.m_depth = m_params.m_depth;
    p.m_depthLayer = m_params.m_depthLayer;
    p.m_join = m_params.m_join;
    p.m_pxHalfWidth = m_params.m_width / 2;
  };

  if (m_params.m_pattern.empty())
  {
    int lineWidth = 1;
    m_isSimple = CanBeSimplified(lineWidth);
    if (m_isSimple)
    {
      SimpleSolidLineBuilder::BuilderParams p;
      commonParamsBuilder(p);

      auto builder = std::make_unique<SimpleSolidLineBuilder>(p, m_spline->GetPath().size(), lineWidth);
      ConstructSimpleSolidLine(m_spline, m_params.m_tileCenter, m_params.m_depth, *builder);
      m_lineShapeInfo = std::move(builder);
    }
    else
    {
      SolidLineBuilder::BuilderParams p;
      commonParamsBuilder(p);

      auto builder = std::make_unique<SolidLineBuilder>(p, m_spline->GetPath().size());
      ConstructSolidLine(m_spline, m_params.m_tileCenter, m_params.m_depth, *builder);
      m_lineShapeInfo = std::move(builder);
    }
  }
  else
  {
    dp::TextureManager::StippleRegion maskRegion;
    textures->GetStippleRegion(m_params.m_pattern, maskRegion);

    DashedLineBuilder::BuilderParams p;
    commonParamsBuilder(p);
    p.m_stipple = maskRegion;
    p.m_baseGtoP = static_cast<float>(m_params.m_baseGtoPScale);

    auto builder = std::make_unique<DashedLineBuilder>(p, m_spline->GetPath().size());
    ConstructDashedLine(m_spline, m_params.m_tileCenter, m_params.m_depth, *builder);
    m_lineShapeInfo = std::move(builder);
  }
}

void LineShape::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                     ref_ptr<dp::TextureManager> textures) const
{
  if (!m_lineShapeInfo)
    Prepare(textures);

  ASSERT(m_lineShapeInfo != nullptr, ());
  dp::RenderState state = m_lineShapeInfo->GetState();
  dp::AttributeProvider provider(1, m_lineShapeInfo->GetLineSize());
  provider.InitStream(0, m_lineShapeInfo->GetBindingInfo(), m_lineShapeInfo->GetLineData());
  if (!m_isSimple)
  {
    batcher->InsertListOfStrip(context, state, make_ref(&provider), dp::Batcher::VertexPerQuad);

    // Not used, keep comment for possible usage. LineJoin::RoundJoin is processed as _Cap_.
    //    uint32_t const joinSize = m_lineShapeInfo->GetJoinSize();
    //    if (joinSize > 0)
    //    {
    //      dp::AttributeProvider joinsProvider(1, joinSize);
    //      joinsProvider.InitStream(0, m_lineShapeInfo->GetBindingInfo(), m_lineShapeInfo->GetJoinData());
    //      batcher->InsertTriangleList(context, state, make_ref(&joinsProvider));
    //    }

    uint32_t const capSize = m_lineShapeInfo->GetCapSize();
    if (capSize > 0)
    {
      dp::AttributeProvider capProvider(1, capSize);
      capProvider.InitStream(0, m_lineShapeInfo->GetCapBindingInfo(), m_lineShapeInfo->GetCapData());
      batcher->InsertTriangleList(context, m_lineShapeInfo->GetCapState(), make_ref(&capProvider));
    }
  }
  else
  {
    batcher->InsertLineStrip(context, state, make_ref(&provider));
  }
}
// =================================================================================================
// RainbowLineShape implementation
// =================================================================================================

namespace
{
// Maximum number of color stripes in a rainbow line, defined by vertex layout
// (two vec4 attributes packing 4 vec2 texture coordinates).
static size_t constexpr kMaxRainbowStripes = 4;

struct RainbowBaseBuilderParams
{
  std::array<dp::TextureManager::ColorRegion, kMaxRainbowStripes> m_colorRegions;
  float m_pxHalfWidth;
  float m_depth;
  bool m_depthTestEnabled;
  DepthLayer m_depthLayer;
  dp::LineCap m_cap;
  dp::LineJoin m_join;
  size_t m_stripeCount;
};

/// Packs up to kMaxRainbowStripes color texture coordinates into two vec4s.
/// Unused slots are filled with the last valid color.
void PackRainbowColorCoords(RainbowBaseBuilderParams const & params, glsl::vec4 & colorCoords01,
                            glsl::vec4 & colorCoords23, glsl::vec2 & stripeInfo)
{
  ASSERT(params.m_stripeCount > 0 && params.m_stripeCount <= kMaxRainbowStripes, (params.m_stripeCount));

  float * dst[kMaxRainbowStripes] = {&colorCoords01.x, &colorCoords01.z, &colorCoords23.x, &colorCoords23.z};
  for (size_t i = 0; i < kMaxRainbowStripes; ++i)
  {
    int const srcIdx = (i < params.m_stripeCount) ? i : (params.m_stripeCount - 1);
    auto const center = params.m_colorRegions[srcIdx].GetTexRect().Center();
    dst[i][0] = center.x;
    dst[i][1] = center.y;
  }
  stripeInfo = {static_cast<float>(params.m_stripeCount), 0.0f};
}

class RainbowSolidLineBuilder : public LineShapeInfo
{
  using V = gpu::RainbowLineVertex;
  using TNormal = gpu::LineVertex::TNormal;
  using TGeometryBuffer = gpu::VBReservedSizeT<V>;

public:
  RainbowSolidLineBuilder(RainbowBaseBuilderParams const & params, size_t pointsInSpline) : m_params(params)
  {
    m_geometry.reserve(pointsInSpline * 2);
    PackRainbowColorCoords(m_params, m_colorCoords01, m_colorCoords23, m_stripeInfo);
  }

  dp::BindingInfo const & GetBindingInfo() override { return V::GetBindingInfo(); }

  dp::RenderState GetState() override
  {
    auto state = CreateRenderState(gpu::Program::RainbowLine, m_params.m_depthLayer);
    state.SetColorTexture(m_params.m_colorRegions[0].GetTexture());
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    return state;
  }

  ref_ptr<void> GetLineData() override { return make_ref(m_geometry.data()); }
  uint32_t GetLineSize() override { return static_cast<uint32_t>(m_geometry.size()); }

  dp::BindingInfo const & GetCapBindingInfo() override { return GetBindingInfo(); }
  dp::RenderState GetCapState() override { return GetState(); }
  ref_ptr<void> GetCapData() override { return ref_ptr<void>(); }
  uint32_t GetCapSize() override { return 0; }

  float GetHalfWidth() const { return m_params.m_pxHalfWidth; }
  float GetSide(bool isLeft) const { return isLeft ? 1.0f : -1.0f; }

  void SubmitVertex(glsl::vec3 const & pivot, glsl::vec2 const & normal, bool isLeft)
  {
    float const halfWidth = m_params.m_pxHalfWidth;
    m_geometry.emplace_back(pivot, TNormal(halfWidth * normal, halfWidth * GetSide(isLeft)), m_colorCoords01,
                            m_colorCoords23, m_stripeInfo);
  }

  void SubmitJoin(glsl::vec2 const &) {}
  void SubmitCap(glsl::vec2 const &) {}

private:
  RainbowBaseBuilderParams m_params;
  TGeometryBuffer m_geometry;
  glsl::vec4 m_colorCoords01{};
  glsl::vec4 m_colorCoords23{};
  glsl::vec2 m_stripeInfo{};
};

class RainbowDashedLineBuilder : public LineShapeInfo
{
  using V = gpu::DashedRainbowLineVertex;
  using TNormal = gpu::LineVertex::TNormal;
  using TGeometryBuffer = gpu::VBReservedSizeT<V>;

public:
  struct BuilderParams : RainbowBaseBuilderParams
  {
    dp::TextureManager::StippleRegion m_stipple;
    float m_baseGtoP;
  };

  RainbowDashedLineBuilder(BuilderParams const & params, size_t pointsInSpline)
    : m_params(params)
    , m_texCoordGen(params.m_stipple)
    , m_baseGtoPScale(params.m_baseGtoP)
  {
    m_geometry.reserve(pointsInSpline * 8);
    PackRainbowColorCoords(m_params, m_colorCoords01, m_colorCoords23, m_stripeInfo);
  }

  float GetMaskLengthG() const { return m_texCoordGen.GetMaskLength() / m_baseGtoPScale; }

  dp::BindingInfo const & GetBindingInfo() override { return V::GetBindingInfo(); }

  dp::RenderState GetState() override
  {
    auto state = CreateRenderState(gpu::Program::DashedRainbowLine, m_params.m_depthLayer);
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    state.SetColorTexture(m_params.m_colorRegions[0].GetTexture());
    state.SetMaskTexture(m_texCoordGen.GetRegion().GetTexture());
    return state;
  }

  ref_ptr<void> GetLineData() override { return make_ref(m_geometry.data()); }
  uint32_t GetLineSize() override { return static_cast<uint32_t>(m_geometry.size()); }

  dp::BindingInfo const & GetCapBindingInfo() override { return GetBindingInfo(); }
  dp::RenderState GetCapState() override { return GetState(); }
  ref_ptr<void> GetCapData() override { return ref_ptr<void>(); }
  uint32_t GetCapSize() override { return 0; }

  float GetHalfWidth() const { return m_params.m_pxHalfWidth; }
  float GetSide(bool isLeft) const { return isLeft ? 1.0f : -1.0f; }

  void SubmitVertex(glsl::vec3 const & pivot, glsl::vec2 const & normal, bool isLeft, float offsetFromStart)
  {
    float const halfWidth = m_params.m_pxHalfWidth;
    m_geometry.emplace_back(pivot, TNormal(halfWidth * normal, halfWidth * GetSide(isLeft)), m_colorCoords01,
                            m_colorCoords23, m_texCoordGen.GetTexCoordsByDistance(offsetFromStart, isLeft),
                            m_stripeInfo);
  }

private:
  BuilderParams m_params;
  TGeometryBuffer m_geometry;
  TextureCoordGenerator m_texCoordGen;
  float const m_baseGtoPScale;
  glsl::vec4 m_colorCoords01{};
  glsl::vec4 m_colorCoords23{};
  glsl::vec2 m_stripeInfo{};
};
}  // namespace

RainbowLineShape::RainbowLineShape(m2::SharedSpline const & spline, RainbowLineViewParams const & params)
  : m_params(params)
  , m_spline(spline)
{
  ASSERT_GREATER(m_spline->GetPath().size(), 1, ());
}

void RainbowLineShape::Prepare(ref_ptr<dp::TextureManager> textures) const
{
  ASSERT(!m_params.m_colors.empty(), ());

  size_t const stripeCount = std::min(m_params.m_colors.size(), kMaxRainbowStripes);
  float const totalWidth = m_params.m_stripeWidth * stripeCount;

  auto commonParamsBuilder = [&](RainbowBaseBuilderParams & p)
  {
    for (size_t i = 0; i < stripeCount; ++i)
      textures->GetColorRegion(m_params.m_colors[i], p.m_colorRegions[i]);

    p.m_cap = m_params.m_cap;
    p.m_join = m_params.m_join;
    p.m_depthTestEnabled = m_params.m_depthTestEnabled;
    p.m_depth = m_params.m_depth;
    p.m_depthLayer = m_params.m_depthLayer;
    p.m_pxHalfWidth = totalWidth / 2;
    p.m_stripeCount = stripeCount;
  };

  if (m_params.m_pattern.empty())
  {
    RainbowBaseBuilderParams p;
    commonParamsBuilder(p);

    auto builder = std::make_unique<RainbowSolidLineBuilder>(p, m_spline->GetPath().size());
    ConstructSolidLine(m_spline, m_params.m_tileCenter, m_params.m_depth, *builder);
    m_lineShapeInfo = std::move(builder);
  }
  else
  {
    dp::TextureManager::StippleRegion maskRegion;
    textures->GetStippleRegion(m_params.m_pattern, maskRegion);

    RainbowDashedLineBuilder::BuilderParams p;
    commonParamsBuilder(p);
    p.m_stipple = maskRegion;
    p.m_baseGtoP = static_cast<float>(m_params.m_baseGtoPScale);

    auto builder = std::make_unique<RainbowDashedLineBuilder>(p, m_spline->GetPath().size());
    ConstructDashedLine(m_spline, m_params.m_tileCenter, m_params.m_depth, *builder);
    m_lineShapeInfo = std::move(builder);
  }
}

void RainbowLineShape::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                            ref_ptr<dp::TextureManager> textures) const
{
  if (!m_lineShapeInfo)
    Prepare(textures);

  ASSERT(m_lineShapeInfo != nullptr, ());
  dp::RenderState state = m_lineShapeInfo->GetState();
  dp::AttributeProvider provider(1, m_lineShapeInfo->GetLineSize());
  provider.InitStream(0, m_lineShapeInfo->GetBindingInfo(), m_lineShapeInfo->GetLineData());
  batcher->InsertListOfStrip(context, state, make_ref(&provider), dp::Batcher::VertexPerQuad);
}

}  // namespace df
