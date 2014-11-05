#include "line_shape.hpp"

#include "../drape/glsl_types.hpp"
#include "../drape/glsl_func.hpp"
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

namespace df
{

namespace
{

class GeomHelper
{
public:
  GeomHelper(glsl::vec2 const & prevPt,
             glsl::vec2 const & curPt,
             glsl::vec2 const & nextPt)
    : m_prevPt(prevPt)
    , m_currentPt(curPt)
    , m_nextPt(nextPt)
  {
    Init(m_prevPt, m_currentPt, m_nextPt);
  }

  void MoveNext(glsl::vec2 const & nextPt)
  {
    m_prevPt = m_currentPt;
    m_currentPt = m_nextPt;
    m_nextPt = nextPt;
    Init(m_prevPt, m_currentPt, m_nextPt);
  }

  glsl::vec2 m_prevPt, m_currentPt, m_nextPt;
  glsl::vec2 m_leftBisector, m_rightBisector;
  float m_dx;
  float m_aspect;

private:
  /// Split angle v1-v2-v3 by bisector.
  void Init(glsl::vec2 const & v1, glsl::vec2 const & v2, glsl::vec2 const & v3)
  {
    glsl::vec2 const dif21 = v2 - v1;
    glsl::vec2 const dif32 = v3 - v2;

    float const l1 = glsl::length(dif21);
    float const l2 = glsl::length(dif32);
    float const l3 = glsl::length(v1 - v3);
    m_aspect = l1 / l2;

    /// normal1 is normal from segment v2 - v1
    /// normal2 is normal from segmeny v3 - v2
    glsl::vec2 const normal1(-dif21.y / l1, dif21.x / l1);
    glsl::vec2 const normal2(-dif32.y / l2, dif32.x / l2);

    m_leftBisector = normal1 + normal2;

    /// find cos(2a), where angle a is a half of v1-v2-v3 angle
    float const cos2A = (l1 * l1 + l2 * l2 - l3 * l3) / (2.0f * l1 * l2);
    float const sinA = sqrt((1.0f - cos2A) / 2.0f);

    m_leftBisector = m_leftBisector / (sinA * glsl::length(m_leftBisector));
    m_rightBisector = -m_leftBisector;

    float offsetLength = glsl::length(m_leftBisector - normal1);

    float const sign = (glsl::dot(dif21, m_leftBisector) > 0.0f) ? 1.0f : -1.0f;
    m_dx = sign * 2.0f * (offsetLength / l1);

    m_leftBisector += v2;
    m_rightBisector += v2;
  }
};

class LineEnumerator
{
public:
  explicit LineEnumerator(vector<m2::PointD> const & points, float GtoPScale, bool generateEndpoints)
    : m_points(points)
    , m_GtoPScale(GtoPScale)
    , m_generateEndpoints(generateEndpoints)
  {
    ASSERT_GREATER(points.size(), 1, ());
    if (generateEndpoints)
      m_iterIndex = -1;
  }

  bool IsSolid() const
  {
    return m_isSolid;
  }

  int32_t GetTextureSet() const
  {
    ASSERT(m_colorRegion.IsValid(), ());
    ASSERT(m_stippleRegion.IsValid(), ());
    ASSERT(m_colorRegion.GetTextureNode().m_textureSet == m_stippleRegion.GetTextureNode().m_textureSet, ());
    return m_colorRegion.GetTextureNode().m_textureSet;
  }

  void SetColorParams(dp::TextureSetHolder::ColorRegion const & color)
  {
    m_colorRegion = color;
  }

  glsl::vec3 GetColorItem() const
  {
    ASSERT(m_colorRegion.IsValid(), ());
    m2::PointF const texCoord = m_colorRegion.GetTexRect().Center();
    return glsl::vec3(texCoord.x, texCoord.y, m_colorRegion.GetTextureNode().GetOffset());
  }

  void SetMaskParams(dp::TextureSetHolder::StippleRegion const & stippleRegion, bool isSolid)
  {
    m_stippleRegion = stippleRegion;
    m_isSolid = isSolid;
  }

  dp::TextureSetHolder::StippleRegion const & GetStippleMaskItem()
  {
    ASSERT(!IsSolid(), ());
    ASSERT(m_stippleRegion.IsValid(), ());
    return m_stippleRegion;
  }

  glsl::vec3 GetSolidMaskItem() const
  {
    ASSERT(IsSolid(), ());
    ASSERT(m_stippleRegion.IsValid(), ());
    m2::PointF const texCoord = m_stippleRegion.GetTexRect().Center();
    return glsl::vec3(texCoord.x, texCoord.y, m_stippleRegion.GetTextureNode().GetOffset());
  }

  glsl::vec2 GetNextPoint()
  {
    ASSERT(HasPoint(), ());
    glsl::vec2 result;
    if (IsSolid())
      GetNextSolid(result);
    else
      GetNextStipple(result);

    return result;
  }

  bool HasPoint() const
  {
    int additionalSize = m_generateEndpoints ? 1 : 0;
    return m_iterIndex < static_cast<int>(m_points.size() + additionalSize);
  }

private:
  void GetNextSolid(glsl::vec2 & result)
  {
    if (GetGeneratedStart(result) || GetGeneratedEnd(result))
      return;

    result = glsl::ToVec2(m_points[m_iterIndex++]);
  }

  void GetNextStipple(glsl::vec2 & result)
  {
    if (GetGeneratedStart(result) || GetGeneratedEnd(result))
      return;

    if (m_iterIndex == 0)
    {
      result = glsl::ToVec2(m_points[m_iterIndex++]);
      return;
    }

    m2::PointD const & currentPt = m_points[m_iterIndex];
    m2::PointD const & prevPt = m_points[m_iterIndex - 1];
    m2::PointD const segmentVector = currentPt - prevPt;

    float const pixelLength = segmentVector.Length() * m_GtoPScale;
    int const maskPixelLength = m_stippleRegion.GetMaskPixelLength();
    int const patternPixelLength = m_stippleRegion.GetPatternPixelLength();

    bool partialSegment = false;
    int maxLength = maskPixelLength;
    int pixelDiff = maskPixelLength - patternPixelLength;
    int resetType = 0;
    if (pixelLength > pixelDiff)
    {
      partialSegment = true;
      maxLength = pixelDiff;
      resetType = 1;
    }
    else if (pixelDiff == 0 && pixelLength > maskPixelLength)
      partialSegment = true;

    if (partialSegment)
    {
      int const numParts = static_cast<int>(ceilf(pixelLength / maxLength));
      m2::PointD const singlePart = segmentVector / (float)numParts;

      result = glsl::ToVec2(prevPt + singlePart * m_partGenerated);

      m_partGenerated++;
      if (resetType == 0 && m_partGenerated == numParts)
      {
        m_partGenerated = 0;
        m_iterIndex++;
      }
      else if (resetType == 1 && m_partGenerated > numParts)
      {
        m_partGenerated = 1;
        m_iterIndex++;
      }
    }
    else
      result = glsl::ToVec2(m_points[m_iterIndex++]);
  }

  bool GetGeneratedStart(glsl::vec2 & result)
  {
    if (m_iterIndex == -1)
    {
      ASSERT(m_generateEndpoints, ());
      glsl::vec2 tmp = glsl::ToVec2(m_points[0]);
      result = tmp + glsl::normalize(tmp - glsl::ToVec2(m_points[1]));
      m_iterIndex = 0;
      return true;
    }

    return false;
  }

  bool GetGeneratedEnd(glsl::vec2 & result)
  {
    if (m_iterIndex == static_cast<int>(m_points.size()))
    {
      ASSERT(m_generateEndpoints, ());
      size_t size = m_points.size();
      glsl::vec2 tmp = glsl::ToVec2(m_points[size - 1]);
      result = tmp + glsl::normalize(tmp - glsl::ToVec2(m_points[size - 2]));
      m_iterIndex++;
      return true;
    }

    return false;
  }

private:
  vector<m2::PointD> const & m_points;
  int m_iterIndex = 0;

  dp::TextureSetHolder::ColorRegion m_colorRegion;
  dp::TextureSetHolder::StippleRegion m_stippleRegion;
  bool m_isSolid = true;

  float m_GtoPScale = 1.0f;
  bool m_generateEndpoints = false;
  int m_partGenerated = 1;
};

} // namespace

LineShape::LineShape(m2::SharedSpline const & spline,
                     LineViewParams const & params)
  : m_params(params)
  , m_spline(spline)
{
  ASSERT_GREATER(m_spline->GetPath().size(), 1, ());
}

void LineShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  LineEnumerator enumerator(m_spline->GetPath(), m_params.m_baseGtoPScale, m_params.m_cap != dp::ButtCap);

  dp::TextureSetHolder::ColorRegion colorRegion;
  textures->GetColorRegion(dp::ColorKey(m_params.m_color.GetColorInInt()), colorRegion);
  enumerator.SetColorParams(colorRegion);

  dp::StipplePenKey maskKey;
  if (!m_params.m_pattern.empty())
    maskKey = dp::StipplePenKey(m_params.m_pattern);
  else
    maskKey = dp::StipplePenKey::Solid();

  dp::TextureSetHolder::StippleRegion maskRegion;
  textures->GetStippleRegion(maskKey, maskRegion);
  enumerator.SetMaskParams(maskRegion, m_params.m_pattern.empty());

  float const joinType = m_params.m_join == dp::RoundJoin ? 1 : 0;
  float const capType = m_params.m_cap == dp::RoundCap ? -1 : 1;
  float const halfWidth = m_params.m_width / 2.0f;
  float const insetHalfWidth= 1.0f * halfWidth;

  int size = m_spline->GetPath().size();
  if (m_params.m_cap != dp::ButtCap)
    size += 2;

  // We generate quad for each line segment
  // For line with 3 point A -- B -- C, we will generate
  // A1 --- AB1 BC1 --- C1
  // |       |   |      |
  // A2 --- AB2 BC2 --- C2
  // By this segment count == points.size() - 1
  int vertexCount = (size - 1) * 4;
  vector<glsl::vec4> vertexArray;
  vector<glsl::vec3> dxValsArray;
  vector<glsl::vec4> centersArray;
  vector<glsl::vec4> widthTypeArray;
  vector<glsl::vec3> colorArray;
  vector<glsl::vec3> maskArray;

  vertexArray.reserve(vertexCount);
  dxValsArray.reserve(vertexCount);
  centersArray.reserve(vertexCount);
  widthTypeArray.reserve(vertexCount);

  glsl::vec2 v2 = enumerator.GetNextPoint();
  glsl::vec2 v3 = enumerator.GetNextPoint();
  glsl::vec2 v1 = 2.0f * v2 - v3;

  GeomHelper helper(v1, v2, v3);

  // Fill first two vertex of segment. A1 and A2
  if (m_params.m_cap != dp::ButtCap)
  {
    vertexArray.push_back(glsl::vec4(helper.m_nextPt, helper.m_leftBisector));
    vertexArray.push_back(glsl::vec4(helper.m_nextPt, helper.m_rightBisector));
    widthTypeArray.push_back(glsl::vec4(halfWidth, capType, -1, insetHalfWidth));
    widthTypeArray.push_back(glsl::vec4(-halfWidth, capType, -1, insetHalfWidth));
  }
  else
  {
    vertexArray.push_back(glsl::vec4(helper.m_currentPt, helper.m_leftBisector));
    vertexArray.push_back(glsl::vec4(helper.m_currentPt, helper.m_rightBisector));
    widthTypeArray.push_back(glsl::vec4(halfWidth, 0, joinType, insetHalfWidth));
    widthTypeArray.push_back(glsl::vec4(-halfWidth, 0, joinType, insetHalfWidth));
  }

  dxValsArray.push_back(glsl::vec3(0.0f, -1.0f, m_params.m_depth));
  dxValsArray.push_back(glsl::vec3(0.0f, -1.0f, m_params.m_depth));
  centersArray.push_back(glsl::vec4(helper.m_currentPt, helper.m_nextPt));
  centersArray.push_back(glsl::vec4(helper.m_currentPt, helper.m_nextPt));

  //points in the middle
  while (enumerator.HasPoint())
  {
    helper.MoveNext(enumerator.GetNextPoint());
    // In line like A -- AB BC -- C we fill AB points in this block
    vertexArray.push_back(glsl::vec4(helper.m_currentPt, helper.m_leftBisector));
    vertexArray.push_back(glsl::vec4(helper.m_currentPt, helper.m_rightBisector));
    dxValsArray.push_back(glsl::vec3(helper.m_dx, 1.0f, m_params.m_depth));
    dxValsArray.push_back(glsl::vec3(-helper.m_dx, 1.0f, m_params.m_depth));
    centersArray.push_back(glsl::vec4(helper.m_prevPt, helper.m_currentPt));
    centersArray.push_back(glsl::vec4(helper.m_prevPt, helper.m_currentPt));
    widthTypeArray.push_back(glsl::vec4(halfWidth, 0, joinType, insetHalfWidth));
    widthTypeArray.push_back(glsl::vec4(-halfWidth, 0, joinType, insetHalfWidth));

    // As AB and BC point calculation on the same source point B
    // we also fill BC points
    vertexArray.push_back(glsl::vec4(helper.m_currentPt, helper.m_leftBisector));
    vertexArray.push_back(glsl::vec4(helper.m_currentPt, helper.m_rightBisector));
    dxValsArray.push_back(glsl::vec3(-helper.m_dx * helper.m_aspect, -1.0f, m_params.m_depth));
    dxValsArray.push_back(glsl::vec3(helper.m_dx * helper.m_aspect, -1.0f, m_params.m_depth));
    centersArray.push_back(glsl::vec4(helper.m_currentPt, helper.m_nextPt));
    centersArray.push_back(glsl::vec4(helper.m_currentPt, helper.m_nextPt));
    widthTypeArray.push_back(glsl::vec4(halfWidth, 0, joinType, insetHalfWidth));
    widthTypeArray.push_back(glsl::vec4(-halfWidth, 0, joinType, insetHalfWidth));
  }

  // Here we fill last vertexes of last segment
  v1 = helper.m_currentPt;
  v2 = helper.m_nextPt;
  v3 = 2.0f * v2 - v1;
  helper = GeomHelper(v1, v2, v3);

  if (m_params.m_cap !=dp::ButtCap)
  {
    vertexArray.push_back(glsl::vec4(helper.m_prevPt, helper.m_leftBisector));
    vertexArray.push_back(glsl::vec4(helper.m_prevPt, helper.m_rightBisector));
    widthTypeArray.push_back(glsl::vec4(halfWidth, capType, 1, insetHalfWidth));
    widthTypeArray.push_back(glsl::vec4(-halfWidth, capType, 1, insetHalfWidth));
  }
  else
  {
    vertexArray.push_back(glsl::vec4(helper.m_currentPt, helper.m_leftBisector));
    vertexArray.push_back(glsl::vec4(helper.m_currentPt, helper.m_rightBisector));
    widthTypeArray.push_back(glsl::vec4(halfWidth, 0, joinType, insetHalfWidth));
    widthTypeArray.push_back(glsl::vec4(-halfWidth, 0, joinType, insetHalfWidth));
  }

  dxValsArray.push_back(glsl::vec3(-helper.m_dx * helper.m_aspect, 1.0f, m_params.m_depth));
  dxValsArray.push_back(glsl::vec3(helper.m_dx * helper.m_aspect, 1.0f, m_params.m_depth));
  centersArray.push_back(glsl::vec4(helper.m_prevPt, helper.m_currentPt));
  centersArray.push_back(glsl::vec4(helper.m_prevPt, helper.m_currentPt));

  ASSERT(vertexArray.size() == dxValsArray.size(), ());
  ASSERT(vertexArray.size() == centersArray.size(), ());
  ASSERT(vertexArray.size() == widthTypeArray.size(), ());
  ASSERT(vertexArray.size() % 4 == 0, ());

  colorArray.resize(vertexArray.size(), enumerator.GetColorItem());

  if (!enumerator.IsSolid())
  {
    dp::TextureSetHolder::StippleRegion const & region = enumerator.GetStippleMaskItem();
    float const stippleTexOffset = region.GetTextureNode().GetOffset();
    m2::RectF const & stippleRect = region.GetTexRect();

    float maskTexLength = region.GetMaskPixelLength() / stippleRect.SizeX();
    float patternTexLength = region.GetPatternPixelLength() / maskTexLength;
    float const dxFactor = halfWidth / (2.0f * m_params.m_baseGtoPScale);
    float patternStart = 0.0f;
    int quadCount = vertexArray.size() / 4;
    for(int i = 0; i < quadCount; ++i)
    {
      int const baseIndex = i * 4;
      float dx1 = dxValsArray[baseIndex + 0].x * dxFactor;
      float dx2 = dxValsArray[baseIndex + 1].x * dxFactor;
      float dx3 = dxValsArray[baseIndex + 2].x * dxFactor;
      float dx4 = dxValsArray[baseIndex + 3].x * dxFactor;
      float const lengthPx = glsl::length(vertexArray[baseIndex].xy() - vertexArray[baseIndex + 2].xy());
      float const lengthTex = lengthPx * m_params.m_baseGtoPScale / (maskTexLength * (fabs(dx1) + fabs(dx3) + 1.0));
      float const f1 = fabs(dx1) * lengthTex;
      float const length2 = (fabs(dx1) + 1.0) * lengthTex;

      dx1 *= lengthTex;
      dx2 *= lengthTex;
      dx3 *= lengthTex;
      dx4 *= lengthTex;

      maskArray.push_back(glsl::vec3(stippleRect.minX() + patternStart + dx1 + f1, stippleRect.minY(), stippleTexOffset));
      maskArray.push_back(glsl::vec3(stippleRect.minX() + patternStart + dx2 + f1, stippleRect.maxY(), stippleTexOffset));
      maskArray.push_back(glsl::vec3(stippleRect.minX() + patternStart + length2 + dx3, stippleRect.minY(), stippleTexOffset));
      maskArray.push_back(glsl::vec3(stippleRect.minX() + patternStart + length2 + dx4, stippleRect.maxY(), stippleTexOffset));

      patternStart += length2;
      while (patternStart >= patternTexLength)
        patternStart -= patternTexLength;
    }
  }
  else
    maskArray.resize(vertexArray.size(), enumerator.GetSolidMaskItem());

  dp::GLState state(gpu::SOLID_LINE_PROGRAM, dp::GLState::GeometryLayer);
  state.SetTextureSet(enumerator.GetTextureSet());
  state.SetBlending(dp::Blending(true));

  dp::AttributeProvider provider(6, vertexArray.size());

  {
    dp::BindingInfo pos_dir(1);
    dp::BindingDecl & decl = pos_dir.GetBindingDecl(0);
    decl.m_attributeName = "a_position";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(0, pos_dir, dp::MakeStackRefPointer((void*)vertexArray.data()));
  }
  {
    dp::BindingInfo deltas(1);
    dp::BindingDecl & decl = deltas.GetBindingDecl(0);
    decl.m_attributeName = "a_deltas";
    decl.m_componentCount = 3;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(1, deltas, dp::MakeStackRefPointer((void*)dxValsArray.data()));
  }
  {
    dp::BindingInfo width_type(1);
    dp::BindingDecl & decl = width_type.GetBindingDecl(0);
    decl.m_attributeName = "a_width_type";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(2, width_type, dp::MakeStackRefPointer((void*)widthTypeArray.data()));
  }
  {
    dp::BindingInfo centres(1);
    dp::BindingDecl & decl = centres.GetBindingDecl(0);
    decl.m_attributeName = "a_centres";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(3, centres, dp::MakeStackRefPointer((void*)centersArray.data()));
  }
  {
    dp::BindingInfo colorBinding(1);
    dp::BindingDecl & decl = colorBinding.GetBindingDecl(0);
    decl.m_attributeName = "a_color";
    decl.m_componentCount = 3;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(4, colorBinding, dp::MakeStackRefPointer((void*)colorArray.data()));
  }

  {
    dp::BindingInfo ind(1);
    dp::BindingDecl & decl = ind.GetBindingDecl(0);
    decl.m_attributeName = "a_mask";
    decl.m_componentCount = 3;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    provider.InitStream(5, ind, dp::MakeStackRefPointer((void*)maskArray.data()));
  }

  batcher->InsertListOfStrip(state, dp::MakeStackRefPointer(&provider), 4);
}

} // namespace df

