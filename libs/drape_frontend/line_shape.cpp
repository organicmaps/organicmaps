#include "drape_frontend/line_shape.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/glsl_types.hpp"
#include "drape/support_manager.hpp"
#include "drape/texture_manager.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "indexer/scales.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cmath>
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
  bool m_blendInPaintOrder = false;

  /// Optional rainbow mode: per-side color UV coords for a color strip texture.
  /// When m_hasRainbow is true, left vertices use m_colorCoordLeft and right use m_colorCoordRight.
  bool m_hasRainbow = false;
  ref_ptr<dp::Texture> m_rainbowTexture;
  glsl::vec2 m_colorCoordLeft;
  glsl::vec2 m_colorCoordRight;
};

template <typename TVertex>
class BaseLineBuilder : public LineShapeInfo
{
public:
  BaseLineBuilder(BaseBuilderParams const & params, size_t geomsSize)
    : m_params(params)
    , m_colorCoord(glsl::ToVec2(params.m_color.GetTexRect().Center()))
    , m_colorCoordLeft(params.m_hasRainbow ? params.m_colorCoordLeft : m_colorCoord)
    , m_colorCoordRight(params.m_hasRainbow ? params.m_colorCoordRight : m_colorCoord)
  {
    m_geometry.reserve(geomsSize);
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
  glsl::vec2 const m_colorCoordLeft;
  glsl::vec2 const m_colorCoordRight;
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

  SolidLineBuilder(BuilderParams const & params, size_t pointsInSpline) : TBase(params, (pointsInSpline - 1) * 4) {}

  dp::RenderState GetState() override
  {
    auto state = CreateRenderState(gpu::Program::Line, m_params.m_depthLayer);
    state.SetColorTexture(m_params.m_hasRainbow ? m_params.m_rainbowTexture : m_params.m_color.GetTexture());
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    if (m_params.m_blendInPaintOrder)
      state.SetDepthFunction(dp::TestFunction::Less);
    return state;
  }

  dp::BindingInfo const & GetCapBindingInfo() override
  {
    if (m_params.m_blendInPaintOrder)
      return GetBindingInfo();
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
    if (m_params.m_blendInPaintOrder)
      return GetState();
    ASSERT(!m_capGeometry.empty(), ());

    auto state = CreateRenderState(gpu::Program::CapJoin, m_params.m_depthLayer);
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    state.SetColorTexture(m_params.m_color.GetTexture());
    state.SetDepthFunction(dp::TestFunction::Less);
    return state;
  }

  ref_ptr<void> GetCapData() override
  {
    return m_params.m_blendInPaintOrder ? make_ref<void>(m_fanGeometry.data()) : make_ref<void>(m_capGeometry.data());
  }

  uint32_t GetCapSize() override
  {
    return static_cast<uint32_t>(m_params.m_blendInPaintOrder ? m_fanGeometry.size() : m_capGeometry.size());
  }

  void SubmitVertex(glsl::vec3 const & pivot, glsl::vec2 const & normal, bool isLeft)
  {
    float const halfWidth = GetHalfWidth();
    m_geometry.emplace_back(pivot, TNormal(halfWidth * normal, halfWidth * GetSide(isLeft)),
                            isLeft ? m_colorCoordLeft : m_colorCoordRight);
  }

  void SubmitJoin(glsl::vec2 const & pos, glsl::vec2 const & tangentIn, glsl::vec2 const & tangentOut)
  {
    if (m_params.m_join != dp::RoundJoin)
      return;
    if (!m_params.m_blendInPaintOrder)
      return CreateRoundCap(pos);

    // Covers the gap that the segments leave outside the turn. The side and the sweep direction take one sign, since
    // a reversal makes cross a zero of either sign, while either side fits it.
    float const cross = tangentIn.x * tangentOut.y - tangentIn.y * tangentOut.x;
    bool const left = cross >= 0;
    glsl::vec2 const outer = left ? glsl::vec2(tangentIn.y, -tangentIn.x) : glsl::vec2(-tangentIn.y, tangentIn.x);
    float const angle = std::atan2(std::fabs(cross), glsl::dot(tangentIn, tangentOut));
    CreateRoundFan(pos, outer, left ? angle : -angle);
  }

  void SubmitCap(glsl::vec2 const & pos, bool hasEndJoin = false)
  {
    if (m_params.m_cap == dp::RoundCap || (hasEndJoin && m_params.m_join == dp::RoundJoin))
      CreateRoundCap(pos);
  }

private:
  // Unlike caps of their own program, line triangles keep the paint order of translucent lines in one bucket.
  void CreateRoundFan(glsl::vec2 const & pos, glsl::vec2 const & normal, float angle)
  {
    float constexpr kMaxStep = static_cast<float>(math::pi) / 8;
    int const steps = static_cast<int>(std::ceil(std::fabs(angle) / kMaxStep));
    float const halfWidth = GetHalfWidth();
    glsl::vec3 const center(pos, m_params.m_depth);
    glsl::vec2 rim = normal;
    for (int i = 1; i <= steps; ++i)
    {
      float const a = angle * i / steps;
      glsl::vec2 const next(normal.x * std::cos(a) - normal.y * std::sin(a),
                            normal.x * std::sin(a) + normal.y * std::cos(a));
      // Clockwise like the segments, since back faces are culled. Unlike cap offsets, the normals turn with the map.
      auto const & first = angle > 0 ? next : rim;
      auto const & second = angle > 0 ? rim : next;
      m_fanGeometry.emplace_back(center, TNormal(0, 0, 0), m_colorCoord);
      m_fanGeometry.emplace_back(center, TNormal(halfWidth * first, halfWidth), m_colorCoord);
      m_fanGeometry.emplace_back(center, TNormal(halfWidth * second, halfWidth), m_colorCoord);
      rim = next;
    }
  }

  void CreateRoundCap(glsl::vec2 const & pos)
  {
    ASSERT(!m_params.m_blendInPaintOrder, ("Round disks use a separate program"));
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
  TGeometryBuffer m_fanGeometry;
};

class SimpleSolidLineBuilder : public BaseLineBuilder<gpu::AreaVertex>
{
  using TBase = BaseLineBuilder<gpu::AreaVertex>;

public:
  using BuilderParams = BaseBuilderParams;

  SimpleSolidLineBuilder(BuilderParams const & params, size_t pointsInSpline, int lineWidth)
    : TBase(params, pointsInSpline)
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
    : TBase(params, pointsInSpline * 8)
    , m_texCoordGen(params.m_stipple)
    , m_baseGtoPScale(params.m_baseGtoP)
  {}

  float GetMaskLengthG() const { return m_texCoordGen.GetMaskLength() / m_baseGtoPScale; }

  dp::RenderState GetState() override
  {
    auto state = CreateRenderState(gpu::Program::DashedLine, m_params.m_depthLayer);
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    state.SetColorTexture(m_params.m_hasRainbow ? m_params.m_rainbowTexture : m_params.m_color.GetTexture());
    state.SetMaskTexture(m_texCoordGen.GetRegion().GetTexture());
    return state;
  }

  void SubmitVertex(glsl::vec3 const & pivot, glsl::vec2 const & normal, bool isLeft, float offsetFromStart)
  {
    float const halfWidth = GetHalfWidth();
    m_geometry.emplace_back(pivot, TNormal(halfWidth * normal, halfWidth * GetSide(isLeft)),
                            isLeft ? m_colorCoordLeft : m_colorCoordRight,
                            m_texCoordGen.GetTexCoordsByDistance(offsetFromStart, isLeft));
  }

private:
  TextureCoordGenerator m_texCoordGen;
  float const m_baseGtoPScale;
};

struct SquareDashVertex
{
  glsl::vec3 m_position;
  glsl::vec3 m_normal;  // XY extrudes the edge; Z is its longitudinal extrusion in pixels.
  glsl::vec2 m_color;
  glsl::vec4 m_range;    // Local phase, first and last dash indices, pixel half-width.
  glsl::vec2 m_pattern;  // Painted fraction and period in world units.

  static dp::BindingInfo const & GetBindingInfo()
  {
    static auto const info = []
    {
      dp::BindingFiller<SquareDashVertex> f(5);
      f.FillDecl<glsl::vec3>("a_position");
      f.FillDecl<glsl::vec3>("a_normal");
      f.FillDecl<glsl::vec2>("a_colorTexCoord");
      f.FillDecl<glsl::vec4>("a_range");
      f.FillDecl<glsl::vec2>("a_pattern");
      return f.m_info;
    }();
    return info;
  }
};

// One quad per segment. The shader converts source distances to pixels at the current zoom.
class SquareDashBuilder : public BaseLineBuilder<SquareDashVertex>
{
public:
  SquareDashBuilder(BaseBuilderParams const & params, size_t points, dp::PenPatternT const & pattern, double baseScale)
    : BaseLineBuilder(params, (points - 1) * 4)
    , m_period((pattern[0] + pattern[1]) / baseScale)
    , m_duty(static_cast<double>(pattern[0]) / (pattern[0] + pattern[1]))
  {}

  dp::RenderState GetState() override
  {
    auto state = CreateRenderState(gpu::Program::SquareDash, m_params.m_depthLayer);
    state.SetDepthTestEnabled(m_params.m_depthTestEnabled);
    state.SetColorTexture(m_params.m_hasRainbow ? m_params.m_rainbowTexture : m_params.m_color.GetTexture());
    return state;
  }

  void SubmitSegment(glsl::vec3 const & p1, glsl::vec3 const & p2, glsl::vec2 const & tangent, double distance,
                     double length, double startTrim, double endTrim, bool openStart, bool openEnd)
  {
    // Anchor the pattern at the tile's base scale. Keep fragment positions within one tile of zero
    // even for a short piece millions of pixels from the source start.
    double const phase = std::fmod(distance, m_period) / m_period;
    double const start = phase - startTrim / m_period;
    double const endPhase = phase + length / m_period;
    double const end = endPhase + endTrim / m_period;
    double first = std::floor(start);
    if (first + m_duty <= start)
      ++first;
    double const last = std::ceil(end) - 1;
    if (first > last)
      return;

    // A cut stays flush. A join gets an extension only when a dash actually starts or ends on this segment.
    float const h = GetHalfWidth();
    auto const edge = [&](glsl::vec3 const & pivot, double pos, float extension)
    {
      glsl::vec2 const left = h * glsl::vec2(-tangent.y, tangent.x);
      glsl::vec2 const offset = extension * tangent;
      glsl::vec4 const range(pos, first, last, h);
      glsl::vec2 const pattern(m_duty, m_period);
      m_geometry.push_back({pivot, {offset - left, extension}, m_colorCoordRight, range, pattern});
      m_geometry.push_back({pivot, {offset + left, extension}, m_colorCoordLeft, range, pattern});
    };
    edge(p1, phase, startTrim == 0 && (openStart || first >= start) ? -h : 0);
    edge(p2, endPhase, endTrim == 0 && (openEnd || last + m_duty <= end) ? h : 0);
  }

private:
  double const m_period;
  double const m_duty;
};

template <typename Builder, typename... Args>
void SubmitExtendedEdge(Builder & builder, glsl::vec3 const & pivot, glsl::vec2 const & tangent, int extension,
                        Args... args)
{
  glsl::vec2 const left(-tangent.y, tangent.x);
  glsl::vec2 const offset = static_cast<float>(extension) * tangent;
  builder.SubmitVertex(pivot, offset - left, false, args...);
  builder.SubmitVertex(pivot, offset + left, true, args...);
}
}  // namespace

LineShape::LineShape(m2::SharedSpline const & spline, LineViewParams const & params)
  : LineShape(spline, params, params.m_cap == dp::SquareCap ? m2::SplineClipInfo::Whole(*spline) : m2::SplineClipInfo{})
{}

LineShape::LineShape(m2::SharedSpline const & spline, LineViewParams const & params, m2::SplineClipInfo clip)
  : m_params(params)
  , m_spline(spline)
  , m_clip(clip)
  , m_isSimple(false)
{
  ASSERT_GREATER(m_spline->GetPath().size(), 1, ());
}

template <typename TBuilder>
void LineShape::Construct(TBuilder & builder) const
{
  ASSERT(false, ("No implementation"));
}

template <class FnT>
void LineShape::ForEachSplineSection(FnT && fn) const
{
  std::vector<m2::PointD> const & path = m_spline->GetPath();
  ASSERT(!path.empty(), ());
  size_t const sz = path.size() - 1;
  bool const square = m_params.m_cap == dp::SquareCap;
  // Replacing a run with its chord changes the distances used to place square dashes.
  bool const canMerge = !m_params.m_blendInPaintOrder && !(square && m_params.m_pattern.size() == 2);

  for (size_t i = 0, j = 1; j <= sz; ++j)
  {
    // A solid square cap follows the original end tangent, even at a short turn near the endpoint.
    bool const keepTangent = square && (i == 0 || j + 1 == sz);
    if (canMerge && !keepTangent && j < sz && path[i].EqualDxDy(path[j], kMwmPointAccuracy))
      continue;

    std::pair<m2::PointD, double> tanlen;
    if (j == i + 1)
    {
      // Fast path - take calculated tangent and length.
      tanlen = m_spline->GetTangentAndLength(i);
    }
    else
    {
      tanlen.first = path[j] - path[i];
      tanlen.second = tanlen.first.Length();
      // A closed path, all points of which are too close to its start, has no sections.
      if (tanlen.second == 0)
        continue;
      tanlen.first = tanlen.first / tanlen.second;
    }

    glsl::vec2 const tangent = glsl::ToVec2(tanlen.first);

    bool const first = i == 0;
    bool const last = j == sz;
    fn(ToShapeVertex2(path[i]), ToShapeVertex2(path[j]), tangent, tanlen.second, first, last,
       square && first && m_clip.m_hasStart ? -1 : 0, square && last && m_clip.m_hasEnd ? 1 : 0);

    i = j;
  }
}

// Specialization optimized for dashed lines.
template <>
void LineShape::Construct<DashedLineBuilder>(DashedLineBuilder & builder) const
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
  ForEachSplineSection([&](glsl::vec2 const & p1, glsl::vec2 const & p2, glsl::vec2 const & tangent, float toDraw, bool,
                           bool, int startExtension, int endExtension)
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

      SubmitExtendedEdge(builder, {currPivot, m_params.m_depth}, tangent, startExtension, offset);
      SubmitExtendedEdge(builder, {nextPivot, m_params.m_depth}, tangent, toDraw == 0 ? endExtension : 0, nextOffset);
      startExtension = 0;

      currPivot = nextPivot;
      offset = nextOffset;
      if (offset >= maskLengthG)
        offset = 0;
    }
    while (toDraw > 0);
  });
}

// Specialization optimized for solid lines.
template <>
void LineShape::Construct<SolidLineBuilder>(SolidLineBuilder & builder) const
{
  // Skip joins generation for thin lines, but for paint-order ones, which have butt caps.
  bool const generateJoins = m_params.m_blendInPaintOrder || builder.GetHalfWidth() > 2.5f;

  glsl::vec2 prevTangent(0, 0);
  ForEachSplineSection([&](glsl::vec2 const & p1, glsl::vec2 const & p2, glsl::vec2 const & tangent, double, bool first,
                           bool last, int startExtension, int endExtension)
  {
    SubmitExtendedEdge(builder, {p1, m_params.m_depth}, tangent, startExtension);
    SubmitExtendedEdge(builder, {p2, m_params.m_depth}, tangent, endExtension);

    // Generate joins.
    if (first)
      builder.SubmitCap(p1);
    else if (generateJoins)  // p1 - middle point
      builder.SubmitJoin(p1, prevTangent, tangent);
    if (last)
      builder.SubmitCap(p2, generateJoins && m_clip.m_hasEndJoin);
    prevTangent = tangent;
  });
}

// Specialization optimized for simple solid lines.
template <>
void LineShape::Construct<SimpleSolidLineBuilder>(SimpleSolidLineBuilder & builder) const
{
  std::vector<m2::PointD> const & path = m_spline->GetPath();
  ASSERT_GREATER(path.size(), 1, ());

  // Build geometry.
  for (m2::PointD const & pt : path)
    builder.SubmitVertex(glsl::vec3(ToShapeVertex2(pt), m_params.m_depth));
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
    p.m_blendInPaintOrder = m_params.m_blendInPaintOrder;
    p.m_depth = m_params.m_depth;
    p.m_depthLayer = m_params.m_depthLayer;
    p.m_join = m_params.m_join;
    p.m_pxHalfWidth = m_params.m_width / 2;

    if (!m_params.m_rainbowColors.empty())
    {
      if (auto const rainbow = textures->GetRainbowRegion(m_params.m_rainbowColors))
      {
        p.m_hasRainbow = true;
        p.m_rainbowTexture = rainbow->m_texture;
        p.m_colorCoordLeft = rainbow->m_uvLeft;
        p.m_colorCoordRight = rainbow->m_uvRight;
      }
      // else: atlas full, fall back to single m_color.
    }
  };

  bool const hasRainbow = !m_params.m_rainbowColors.empty();

  // Rainbow stripes need per-fragment color sampling. A fully opaque mask selects the textured
  // line program for a solid rainbow.
  dp::PenPatternT pattern = m_params.m_pattern;
  if (hasRainbow && pattern.empty())
    pattern = {2};  // Single 2px dash, no gap = fully opaque mask.

  ASSERT(m_params.m_cap != dp::SquareCap || pattern.size() <= 2, ());
  if (m_params.m_cap == dp::SquareCap && pattern.size() == 2)
  {
    BaseBuilderParams p;
    commonParamsBuilder(p);
    auto builder = std::make_unique<SquareDashBuilder>(p, m_spline->GetSize(), pattern, m_params.m_baseGtoPScale);
    double distance = m_clip.m_offset;
    ForEachSplineSection([&](glsl::vec2 const & p1, glsl::vec2 const & p2, glsl::vec2 const & tangent, double length,
                             bool first, bool last, int startExtension, int endExtension)
    {
      builder->SubmitSegment({p1, m_params.m_depth}, {p2, m_params.m_depth}, tangent, distance, length,
                             first ? m_clip.m_startTrim : 0, last ? m_clip.m_endTrim : 0, startExtension != 0,
                             endExtension != 0);
      distance += length;
    });
    m_lineShapeInfo = std::move(builder);
  }
  else if (pattern.empty())
  {
    int lineWidth = 1;
    // Native line strips cannot extend square endpoints.
    m_isSimple = m_params.m_cap != dp::SquareCap && !m_params.m_blendInPaintOrder && CanBeSimplified(lineWidth);
    if (m_isSimple)
    {
      SimpleSolidLineBuilder::BuilderParams p;
      commonParamsBuilder(p);

      auto builder = std::make_unique<SimpleSolidLineBuilder>(p, m_spline->GetPath().size(), lineWidth);
      Construct<SimpleSolidLineBuilder>(*builder);
      m_lineShapeInfo = std::move(builder);
    }
    else
    {
      SolidLineBuilder::BuilderParams p;
      commonParamsBuilder(p);

      auto builder = std::make_unique<SolidLineBuilder>(p, m_spline->GetPath().size());
      Construct<SolidLineBuilder>(*builder);
      m_lineShapeInfo = std::move(builder);
    }
  }
  else
  {
    dp::TextureManager::StippleRegion maskRegion;
    textures->GetStippleRegion(pattern, maskRegion);

    DashedLineBuilder::BuilderParams p;
    commonParamsBuilder(p);
    p.m_stipple = maskRegion;
    p.m_baseGtoP = static_cast<float>(m_params.m_baseGtoPScale);

    auto builder = std::make_unique<DashedLineBuilder>(p, m_spline->GetPath().size());
    Construct<DashedLineBuilder>(*builder);
    m_lineShapeInfo = std::move(builder);
  }
}

void LineShape::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                     ref_ptr<dp::TextureManager> textures) const
{
  if (!m_lineShapeInfo)
    Prepare(textures);

  ASSERT(m_lineShapeInfo != nullptr, ());
  auto const vertexCount = m_lineShapeInfo->GetLineSize();
  if (vertexCount == 0)
    return;
  dp::RenderState state = m_lineShapeInfo->GetState();
  dp::AttributeProvider provider(1, vertexCount);
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
}  // namespace df
