#include "line_shape.hpp"

#include "../drape/utils/vertex_decl.hpp"
#include "../drape/glsl_types.hpp"
#include "../drape/glsl_func.hpp"
#include "../drape/shader_def.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"
#include "../drape/texture_manager.hpp"

namespace df
{

namespace
{
  float const SEGMENT = 0.0f;
  float const CAP = 1.0f;
  float const LEFT_WIDTH = 1.0f;
  float const RIGHT_WIDTH = -1.0f;
}

LineShape::LineShape(m2::SharedSpline const & spline,
                     LineViewParams const & params)
  : m_params(params)
  , m_spline(spline)
{
  ASSERT_GREATER(m_spline->GetPath().size(), 1, ());
}

void LineShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureManager> textures) const
{
  buffer_vector<gpu::LineVertex, 128> geometry;
  vector<m2::PointD> const & path = m_spline->GetPath();

  dp::TextureManager::ColorRegion colorRegion;
  textures->GetColorRegion(m_params.m_color, colorRegion);
  glsl::vec2 colorCoord(glsl::ToVec2(colorRegion.GetTexRect().Center()));

  dp::TextureManager::StippleRegion maskRegion;
  textures->GetStippleRegion(dp::TextureManager::TStipplePattern{1}, maskRegion);
  glsl::vec2 maskCoord(glsl::ToVec2(maskRegion.GetTexRect().Center()));

  float const halfWidth = m_params.m_width / 2.0f;
  bool generateCap = m_params.m_cap != dp::ButtCap;

  auto const calcTangentAndNormals = [&halfWidth](glsl::vec2 const & pt0, glsl::vec2 const & pt1,
                                        glsl::vec2 & tangent, glsl::vec2 & leftNormal,
                                        glsl::vec2 & rightNormal)
  {
    tangent = pt1 - pt0;
    leftNormal = halfWidth * glsl::normalize(glsl::vec2(tangent.y, -tangent.x));
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
    tangent = -halfWidth * glsl::normalize(tangent);

    glsl::vec3 pivot = glsl::vec3(startPoint, m_params.m_depth);
    glsl::vec2 leftCap(capType, LEFT_WIDTH);
    glsl::vec2 rightCap(capType, RIGHT_WIDTH);

    geometry.push_back(gpu::LineVertex(pivot, leftNormal + tangent, colorCoord, maskCoord, leftCap));
    geometry.push_back(gpu::LineVertex(pivot, rightNormal + tangent, colorCoord, maskCoord, rightCap));
    geometry.push_back(gpu::LineVertex(pivot, leftNormal, colorCoord, maskCoord, leftSegment));
    geometry.push_back(gpu::LineVertex(pivot, rightNormal, colorCoord, maskCoord, rightSegment));
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
        geometry.push_back(gpu::LineVertex(startPivot, prevForming, colorCoord, maskCoord, leftSegment));
        geometry.push_back(gpu::LineVertex(startPivot, zeroNormal, colorCoord, maskCoord, leftSegment));
        geometry.push_back(gpu::LineVertex(startPivot, nextForming, colorCoord, maskCoord, leftSegment));
        geometry.push_back(gpu::LineVertex(startPivot, nextForming, colorCoord, maskCoord, leftSegment));
      }
      else
      {
        glsl::vec2 middleForming = glsl::normalize(prevForming + nextForming);
        glsl::vec2 zeroDxDy(0.0, 0.0);

        if (m_params.m_join == dp::MiterJoin)
        {
          float const b = glsl::length(prevForming - nextForming) / 2.0;
          float const a = glsl::length(prevForming);
          middleForming *= static_cast<float>(sqrt(a * a + b * b));

          geometry.push_back(gpu::LineVertex(startPivot, prevForming, colorCoord, maskCoord, zeroDxDy));
          geometry.push_back(gpu::LineVertex(startPivot, zeroNormal, colorCoord, maskCoord, zeroDxDy));
          geometry.push_back(gpu::LineVertex(startPivot, middleForming, colorCoord, maskCoord, zeroDxDy));
          geometry.push_back(gpu::LineVertex(startPivot, nextForming, colorCoord, maskCoord, zeroDxDy));
        }
        else
        {
          middleForming *= glsl::length(prevForming);

          glsl::vec2 dxdyLeft(0.0, -1.0);
          glsl::vec2 dxdyRight(0.0, -1.0);
          glsl::vec2 dxdyMiddle(1.0, 1.0);
          geometry.push_back(gpu::LineVertex(startPivot, zeroNormal, colorCoord, maskCoord, zeroDxDy));
          geometry.push_back(gpu::LineVertex(startPivot, prevForming, colorCoord, maskCoord, dxdyLeft));
          geometry.push_back(gpu::LineVertex(startPivot, nextForming, colorCoord, maskCoord, dxdyRight));
          geometry.push_back(gpu::LineVertex(startPivot, middleForming, colorCoord, maskCoord, dxdyMiddle));
        }
      }
    }

    geometry.push_back(gpu::LineVertex(startPivot, leftNormal, colorCoord, maskCoord, leftSegment));
    geometry.push_back(gpu::LineVertex(startPivot, rightNormal, colorCoord, maskCoord, rightSegment));
    geometry.push_back(gpu::LineVertex(endPivot, leftNormal, colorCoord, maskCoord, leftSegment));
    geometry.push_back(gpu::LineVertex(endPivot, rightNormal, colorCoord, maskCoord, rightSegment));

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
    tangent = halfWidth * glsl::normalize(tangent);

    glsl::vec3 pivot = glsl::vec3(endPoint, m_params.m_depth);
    glsl::vec2 leftCap(capType, LEFT_WIDTH);
    glsl::vec2 rightCap(capType, RIGHT_WIDTH);

    geometry.push_back(gpu::LineVertex(pivot, leftNormal, colorCoord, maskCoord, leftSegment));
    geometry.push_back(gpu::LineVertex(pivot, rightNormal, colorCoord, maskCoord, rightSegment));
    geometry.push_back(gpu::LineVertex(pivot, leftNormal + tangent, colorCoord, maskCoord, leftCap));
    geometry.push_back(gpu::LineVertex(pivot, rightNormal + tangent, colorCoord, maskCoord, rightCap));
  }

  dp::GLState state(gpu::LINE_PROGRAM, dp::GLState::GeometryLayer);
  state.SetBlending(true);
  state.SetColorTexture(colorRegion.GetTexture());
  state.SetMaskTexture(maskRegion.GetTexture());

  dp::AttributeProvider provider(1, geometry.size());
  provider.InitStream(0, gpu::LineVertex::GetBindingInfo(), dp::MakeStackRefPointer<void>(geometry.data()));

  batcher->InsertListOfStrip(state, dp::MakeStackRefPointer(&provider), 4);
}

} // namespace df

