#include "drape_frontend/line_shape.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/glsl_types.hpp"
#include "drape/glsl_func.hpp"
#include "drape/shader_def.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/glstate.hpp"
#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"

namespace df
{

namespace
{
  float const SEGMENT = 0.0f;
  float const CAP = 1.0f;
  float const LEFT_WIDTH = 1.0f;
  float const RIGHT_WIDTH = -1.0f;

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
  typedef gpu::LineVertex LV;
  buffer_vector<gpu::LineVertex, 128> geometry;
  vector<m2::PointD> const & path = m_spline->GetPath();

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
  bool generateCap = m_params.m_cap != dp::ButtCap;

  auto const calcTangentAndNormals = [&halfWidth](glsl::vec2 const & pt0, glsl::vec2 const & pt1,
                                        glsl::vec2 & tangent, glsl::vec2 & leftNormal,
                                        glsl::vec2 & rightNormal)
  {
    tangent = glsl::normalize(pt1 - pt0);
    leftNormal = halfWidth * glsl::vec2(tangent.y, -tangent.x);
    rightNormal = -leftNormal;
  };

  float capType = m_params.m_cap == dp::RoundCap ? CAP : SEGMENT;

  glsl::vec2 leftSegment(SEGMENT, LEFT_WIDTH);
  glsl::vec2 rightSegment(SEGMENT, RIGHT_WIDTH);

  if (generateCap)
  {
    glsl::vec2 startPoint = glsl::ToVec2(path[0]);
    glsl::vec2 endPoint = glsl::ToVec2(path[1]);
    glsl::vec2 tangent, leftNormal, rightNormal;
    calcTangentAndNormals(startPoint, endPoint, tangent, leftNormal, rightNormal);
    tangent = -halfWidth * tangent;

    glsl::vec3 pivot = glsl::vec3(startPoint, m_params.m_depth);
    glsl::vec2 leftCap(capType, LEFT_WIDTH);
    glsl::vec2 rightCap(capType, RIGHT_WIDTH);
    glsl::vec2 const uvStart = texCoordGen.GetTexCoordsByDistance(0.0);
    glsl::vec2 const uvEnd = texCoordGen.GetTexCoordsByDistance(glbHalfWidth);

    geometry.push_back(LV(pivot, leftNormal + tangent, colorCoord, uvStart, leftCap));
    geometry.push_back(LV(pivot, rightNormal + tangent, colorCoord, uvStart, rightCap));
    geometry.push_back(LV(pivot, leftNormal, colorCoord, uvEnd, leftSegment));
    geometry.push_back(LV(pivot, rightNormal, colorCoord, uvEnd, rightSegment));
  }

  glsl::vec2 prevPoint;
  glsl::vec2 prevLeftNormal;
  glsl::vec2 prevRightNormal;

  for (size_t i = 1; i < path.size(); ++i)
  {
    glsl::vec2 startPoint = glsl::ToVec2(path[i - 1]);
    glsl::vec2 endPoint = glsl::ToVec2(path[i]);
    glsl::vec2 tangent, leftNormal, rightNormal;
    calcTangentAndNormals(startPoint, endPoint, tangent, leftNormal, rightNormal);

    glsl::vec3 startPivot = glsl::vec3(startPoint, m_params.m_depth);

    // Create join beetween current segment and previous
    if (i > 1)
    {
      glsl::vec2 zeroNormal(0.0, 0.0);
      glsl::vec2 prevForming, nextForming;
      if (glsl::dot(prevLeftNormal, tangent) < 0)
      {
        prevForming = prevLeftNormal;
        nextForming = leftNormal;
      }
      else
      {
        prevForming = prevRightNormal;
        nextForming = rightNormal;
      }

      float const distance = glsl::length(nextForming - prevForming) / m_params.m_baseGtoPScale;
      float const endUv = texCoordGen.GetUvOffsetByDistance(distance);
      glsl::vec2 const uvStart = texCoordGen.GetTexCoords(0.0f);
      glsl::vec2 const uvEnd = texCoordGen.GetTexCoords(endUv);

      if (m_params.m_join == dp::BevelJoin)
      {
        geometry.push_back(LV(startPivot, prevForming, colorCoord, uvStart, leftSegment));
        geometry.push_back(LV(startPivot, zeroNormal, colorCoord, uvStart, leftSegment));
        geometry.push_back(LV(startPivot, nextForming, colorCoord, uvEnd, leftSegment));
        geometry.push_back(LV(startPivot, nextForming, colorCoord, uvEnd, leftSegment));
      }
      else
      {
        glsl::vec2 middleForming = glsl::normalize(prevForming + nextForming);
        glsl::vec2 zeroDxDy(0.0, 0.0);

        float const middleUv = 0.5f * endUv;
        glsl::vec2 const uvMiddle = texCoordGen.GetTexCoords(middleUv);

        if (m_params.m_join == dp::MiterJoin)
        {
          float const b = 0.5f * glsl::length(prevForming - nextForming);
          float const a = glsl::length(prevForming);
          middleForming *= static_cast<float>(sqrt(a * a + b * b));

          geometry.push_back(LV(startPivot, prevForming, colorCoord, uvStart, zeroDxDy));
          geometry.push_back(LV(startPivot, zeroNormal, colorCoord, uvStart, zeroDxDy));
          geometry.push_back(LV(startPivot, middleForming, colorCoord, uvMiddle, zeroDxDy));
          geometry.push_back(LV(startPivot, nextForming, colorCoord, uvEnd, zeroDxDy));
        }
        else
        {
          middleForming *= glsl::length(prevForming);

          glsl::vec2 dxdyLeft(0.0, -1.0);
          glsl::vec2 dxdyRight(0.0, -1.0);
          glsl::vec2 dxdyMiddle(1.0, 1.0);
          geometry.push_back(LV(startPivot, zeroNormal, colorCoord, uvStart, zeroDxDy));
          geometry.push_back(LV(startPivot, prevForming, colorCoord, uvStart, dxdyLeft));
          geometry.push_back(LV(startPivot, nextForming, colorCoord, uvEnd, dxdyRight));
          geometry.push_back(LV(startPivot, middleForming, colorCoord, uvMiddle, dxdyMiddle));
        }
      }
    }

    float initialGlobalLength = glsl::length(endPoint - startPoint);
    float pixelLen = initialGlobalLength * m_params.m_baseGtoPScale;
    int steps = 1;
    float maskSize = initialGlobalLength;
    if (!texCoordGen.IsSolid())
    {
      maskSize = texCoordGen.GetMaskLength() / m_params.m_baseGtoPScale;
      steps = static_cast<int>(pixelLen / texCoordGen.GetMaskLength()) + 1;
    }

    float currentSize = 0;
    glsl::vec3 currentStartPivot = startPivot;
    for (int i = 0; i < steps; i++)
    {
      currentSize += maskSize;

      bool isLastPoint = false;
      float endOffset = 1.0f;
      if (currentSize >= initialGlobalLength)
      {
        endOffset = (initialGlobalLength - currentSize + maskSize) / maskSize;
        currentSize = initialGlobalLength;
        isLastPoint = true;
      }

      glsl::vec2 const newPoint = startPoint + tangent * currentSize;
      glsl::vec3 const newPivot = glsl::vec3(newPoint, m_params.m_depth);
      glsl::vec2 const uvStart = texCoordGen.GetTexCoords(0.0f);
      glsl::vec2 const uvEnd = texCoordGen.GetTexCoords(endOffset);

      geometry.push_back(LV(currentStartPivot, leftNormal, colorCoord, uvStart, leftSegment));
      geometry.push_back(LV(currentStartPivot, rightNormal, colorCoord, uvStart, rightSegment));
      geometry.push_back(LV(newPivot, leftNormal, colorCoord, uvEnd, leftSegment));
      geometry.push_back(LV(newPivot, rightNormal, colorCoord, uvEnd, rightSegment));

      currentStartPivot = newPivot;

      if (isLastPoint)
        break;
    }

    prevPoint = currentStartPivot.xy();
    prevLeftNormal = leftNormal;
    prevRightNormal = rightNormal;
  }

  if (generateCap)
  {
    size_t lastPointIndex = path.size() - 1;
    glsl::vec2 startPoint = glsl::ToVec2(path[lastPointIndex - 1]);
    glsl::vec2 endPoint = glsl::ToVec2(path[lastPointIndex]);
    glsl::vec2 tangent, leftNormal, rightNormal;
    calcTangentAndNormals(startPoint, endPoint, tangent, leftNormal, rightNormal);
    tangent = halfWidth * tangent;

    glsl::vec3 pivot = glsl::vec3(endPoint, m_params.m_depth);
    glsl::vec2 leftCap(capType, LEFT_WIDTH);
    glsl::vec2 rightCap(capType, RIGHT_WIDTH);
    glsl::vec2 const uvStart = texCoordGen.GetTexCoordsByDistance(0.0);
    glsl::vec2 const uvEnd = texCoordGen.GetTexCoordsByDistance(glbHalfWidth);

    geometry.push_back(LV(pivot, leftNormal, colorCoord, uvStart, leftSegment));
    geometry.push_back(LV(pivot, rightNormal, colorCoord, uvStart, rightSegment));
    geometry.push_back(LV(pivot, leftNormal + tangent, colorCoord, uvEnd, leftCap));
    geometry.push_back(LV(pivot, rightNormal + tangent, colorCoord, uvEnd, rightCap));
  }

  dp::GLState state(gpu::LINE_PROGRAM, dp::GLState::GeometryLayer);
  state.SetColorTexture(colorRegion.GetTexture());
  state.SetMaskTexture(maskRegion.GetTexture());

  dp::AttributeProvider provider(1, geometry.size());
  provider.InitStream(0, gpu::LineVertex::GetBindingInfo(), make_ref(geometry.data()));

  batcher->InsertListOfStrip(state, make_ref(&provider), 4);
}

} // namespace df

