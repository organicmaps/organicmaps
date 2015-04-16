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
  size_t const TEX_BEG_IDX = 0;
  size_t const TEX_END_IDX = 1;

  struct TexDescription
  {
    float m_globalLength;
    glsl::vec2 m_texCoord;
  };

  class TextureCoordGenerator
  {
  public:
    TextureCoordGenerator(float const baseGtoPScale)
      : m_baseGtoPScale(baseGtoPScale)
      , m_basePtoGScale(1.0f / baseGtoPScale)
    {
    }

    void SetRegion(dp::TextureManager::StippleRegion const & region, bool isSolid)
    {
      m_isSolid = isSolid;
      m_region = region;
      if (!m_isSolid)
      {
        m_maskLength = static_cast<float>(m_region.GetMaskPixelLength());
        m_patternLength = static_cast<float>(m_region.GetPatternPixelLength());
      }
    }

    bool GetTexCoords(TexDescription & desc)
    {
      if (m_isSolid)
      {
        desc.m_texCoord = glsl::ToVec2(m_region.GetTexRect().Center());
        return true;
      }

      float const pxLength = desc.m_globalLength * m_baseGtoPScale;
      float const maskRest = m_maskLength - m_pxCursor;

      m2::RectF const & texRect = m_region.GetTexRect();
      if (maskRest < pxLength)
      {
        desc.m_globalLength = maskRest * m_basePtoGScale;
        desc.m_texCoord = glsl::vec2(texRect.maxX(), texRect.Center().y);
        return false;
      }

      float texX = texRect.minX() + ((m_pxCursor + pxLength) / m_maskLength) * texRect.SizeX();
      m_pxCursor = fmodf(m_pxCursor + pxLength, m_patternLength);

      desc.m_texCoord = glsl::vec2(texX, texRect.Center().y);
      return true;
    }

  private:
    float const m_baseGtoPScale;
    float const m_basePtoGScale;
    dp::TextureManager::StippleRegion m_region;
    float m_maskLength = 0.0f;
    float m_patternLength = 0.0f;
    bool m_isSolid = true;
    float m_pxCursor = 0.0f;
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

  TexDescription texCoords[2];

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

    texCoords[TEX_BEG_IDX].m_globalLength = 0.0;
    texCoords[TEX_END_IDX].m_globalLength = glbHalfWidth;

    VERIFY(texCoordGen.GetTexCoords(texCoords[TEX_BEG_IDX]), ());
    VERIFY(texCoordGen.GetTexCoords(texCoords[TEX_END_IDX]), ());

    geometry.push_back(LV(pivot, leftNormal + tangent, colorCoord, texCoords[TEX_BEG_IDX].m_texCoord, leftCap));
    geometry.push_back(LV(pivot, rightNormal + tangent, colorCoord, texCoords[TEX_BEG_IDX].m_texCoord, rightCap));
    geometry.push_back(LV(pivot, leftNormal, colorCoord, texCoords[TEX_END_IDX].m_texCoord, leftSegment));
    geometry.push_back(LV(pivot, rightNormal, colorCoord, texCoords[TEX_END_IDX].m_texCoord, rightSegment));
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
    glsl::vec3 endPivot = glsl::vec3(endPoint, m_params.m_depth);

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

      if (m_params.m_join == dp::BevelJoin)
      {
        texCoords[TEX_BEG_IDX].m_globalLength = 0.0f;
        texCoords[TEX_END_IDX].m_globalLength = glsl::length(nextForming - prevForming) / m_params.m_baseGtoPScale;
        VERIFY(texCoordGen.GetTexCoords(texCoords[TEX_BEG_IDX]), ());
        VERIFY(texCoordGen.GetTexCoords(texCoords[TEX_END_IDX]), ());

        geometry.push_back(LV(startPivot, prevForming, colorCoord, texCoords[TEX_BEG_IDX].m_texCoord, leftSegment));
        geometry.push_back(LV(startPivot, zeroNormal, colorCoord, texCoords[TEX_BEG_IDX].m_texCoord, leftSegment));
        geometry.push_back(LV(startPivot, nextForming, colorCoord, texCoords[TEX_END_IDX].m_texCoord, leftSegment));
        geometry.push_back(LV(startPivot, nextForming, colorCoord, texCoords[TEX_END_IDX].m_texCoord, leftSegment));
      }
      else
      {
        glsl::vec2 middleForming = glsl::normalize(prevForming + nextForming);
        glsl::vec2 zeroDxDy(0.0, 0.0);

        texCoords[TEX_BEG_IDX].m_globalLength = 0.0f;
        texCoords[TEX_END_IDX].m_globalLength = glsl::length(nextForming - prevForming) / m_params.m_baseGtoPScale;
        TexDescription middle;
        middle.m_globalLength = texCoords[TEX_END_IDX].m_globalLength / 2.0f;
        VERIFY(texCoordGen.GetTexCoords(texCoords[TEX_BEG_IDX]), ());
        VERIFY(texCoordGen.GetTexCoords(middle), ());
        VERIFY(texCoordGen.GetTexCoords(texCoords[TEX_END_IDX]), ());

        if (m_params.m_join == dp::MiterJoin)
        {
          float const b = glsl::length(prevForming - nextForming) / 2.0;
          float const a = glsl::length(prevForming);
          middleForming *= static_cast<float>(sqrt(a * a + b * b));

          geometry.push_back(LV(startPivot, prevForming, colorCoord, texCoords[TEX_BEG_IDX].m_texCoord, zeroDxDy));
          geometry.push_back(LV(startPivot, zeroNormal, colorCoord, texCoords[TEX_BEG_IDX].m_texCoord, zeroDxDy));
          geometry.push_back(LV(startPivot, middleForming, colorCoord, middle.m_texCoord, zeroDxDy));
          geometry.push_back(LV(startPivot, nextForming, colorCoord, texCoords[TEX_END_IDX].m_texCoord, zeroDxDy));
        }
        else
        {
          middleForming *= glsl::length(prevForming);

          glsl::vec2 dxdyLeft(0.0, -1.0);
          glsl::vec2 dxdyRight(0.0, -1.0);
          glsl::vec2 dxdyMiddle(1.0, 1.0);
          geometry.push_back(LV(startPivot, zeroNormal, colorCoord, texCoords[TEX_BEG_IDX].m_texCoord, zeroDxDy));
          geometry.push_back(LV(startPivot, prevForming, colorCoord, texCoords[TEX_BEG_IDX].m_texCoord, dxdyLeft));
          geometry.push_back(LV(startPivot, nextForming, colorCoord, texCoords[TEX_END_IDX].m_texCoord, dxdyRight));
          geometry.push_back(LV(startPivot, middleForming, colorCoord, middle.m_texCoord, dxdyMiddle));
        }
      }
    }

    texCoords[TEX_BEG_IDX].m_globalLength = 0.0;
    texCoords[TEX_END_IDX].m_globalLength = glsl::length(endPoint - startPoint);
    VERIFY(texCoordGen.GetTexCoords(texCoords[TEX_BEG_IDX]), ());
    while (!texCoordGen.GetTexCoords(texCoords[TEX_END_IDX]))
    {
      glsl::vec2 newEndPoint = startPoint + tangent * texCoords[TEX_END_IDX].m_globalLength;
      glsl::vec3 newEndPivot = glsl::vec3(newEndPoint, m_params.m_depth);

      geometry.push_back(LV(startPivot, leftNormal, colorCoord, texCoords[TEX_BEG_IDX].m_texCoord, leftSegment));
      geometry.push_back(LV(startPivot, rightNormal, colorCoord, texCoords[TEX_BEG_IDX].m_texCoord, rightSegment));
      geometry.push_back(LV(newEndPivot, leftNormal, colorCoord, texCoords[TEX_END_IDX].m_texCoord, leftSegment));
      geometry.push_back(LV(newEndPivot, rightNormal, colorCoord, texCoords[TEX_END_IDX].m_texCoord, rightSegment));

      startPoint = newEndPoint;
      startPivot = newEndPivot;

      VERIFY(texCoordGen.GetTexCoords(texCoords[TEX_BEG_IDX]), ());
      texCoords[TEX_END_IDX].m_globalLength = glsl::length(endPoint - startPoint);
    }

    geometry.push_back(LV(startPivot, leftNormal, colorCoord, texCoords[TEX_BEG_IDX].m_texCoord, leftSegment));
    geometry.push_back(LV(startPivot, rightNormal, colorCoord, texCoords[TEX_BEG_IDX].m_texCoord, rightSegment));
    geometry.push_back(LV(endPivot, leftNormal, colorCoord, texCoords[TEX_END_IDX].m_texCoord, leftSegment));
    geometry.push_back(LV(endPivot, rightNormal, colorCoord, texCoords[TEX_END_IDX].m_texCoord, rightSegment));

    prevPoint = startPoint;
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

    texCoords[TEX_BEG_IDX].m_globalLength = 0.0;
    texCoords[TEX_END_IDX].m_globalLength = glbHalfWidth;

    VERIFY(texCoordGen.GetTexCoords(texCoords[TEX_BEG_IDX]), ());
    VERIFY(texCoordGen.GetTexCoords(texCoords[TEX_END_IDX]), ());

    geometry.push_back(LV(pivot, leftNormal, colorCoord, texCoords[TEX_BEG_IDX].m_texCoord, leftSegment));
    geometry.push_back(LV(pivot, rightNormal, colorCoord, texCoords[TEX_BEG_IDX].m_texCoord, rightSegment));
    geometry.push_back(LV(pivot, leftNormal + tangent, colorCoord, texCoords[TEX_END_IDX].m_texCoord, leftCap));
    geometry.push_back(LV(pivot, rightNormal + tangent, colorCoord, texCoords[TEX_END_IDX].m_texCoord, rightCap));
  }

  dp::GLState state(gpu::LINE_PROGRAM, dp::GLState::GeometryLayer);
  state.SetColorTexture(colorRegion.GetTexture());
  state.SetMaskTexture(maskRegion.GetTexture());

  dp::AttributeProvider provider(1, geometry.size());
  provider.InitStream(0, gpu::LineVertex::GetBindingInfo(), make_ref<void>(geometry.data()));

  batcher->InsertListOfStrip(state, make_ref(&provider), 4);
}

} // namespace df

