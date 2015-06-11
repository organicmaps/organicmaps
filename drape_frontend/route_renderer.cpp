#include "drape_frontend/route_renderer.hpp"

#include "drape/glsl_func.hpp"
#include "drape/shader_def.hpp"
#include "drape/utils/projection.hpp"

#include "indexer/scales.hpp"

#include "base/logging.hpp"

namespace df
{

namespace
{

float const halfWidthInPixel[] =
{
  // 1   2     3     4     5     6     7     8     9     10
  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f, 2.0f,
  //11   12    13    14    15     16    17      18     19
  2.0f, 2.5f, 3.5f, 5.0f, 7.5f, 10.0f, 14.0f, 18.0f, 36.0f,
};

int const arrowPartsCount = 3;
double const arrowHeightFactor = 96.0 / 36.0;
double const arrowAspect = 400.0 / 192.0;
double const arrowTailSize = 32.0 / 400.0;
double const arrowHeadSize = 144.0 / 400.0;

}

RouteGraphics::RouteGraphics(dp::GLState const & state,
                             drape_ptr<dp::VertexArrayBuffer> && buffer,
                             dp::Color const & color)
  : m_state(state)
  , m_buffer(move(buffer))
  , m_color(color)
{}

RouteRenderer::RouteRenderer()
  : m_distanceFromBegin(0.0)
{}

void RouteRenderer::Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
                           dp::UniformValuesStorage const & commonUniforms)
{
  // half width calculation
  float halfWidth = 0.0;
  double const zoomLevel = my::clamp(fabs(log(screen.GetScale()) / log(2.0)), 1.0, scales::UPPER_STYLE_SCALE);
  double const truncedZoom = trunc(zoomLevel);
  int const index = truncedZoom - 1.0;
  float const lerpCoef = zoomLevel - truncedZoom;

  if (index < scales::UPPER_STYLE_SCALE - 1)
    halfWidth = halfWidthInPixel[index] + lerpCoef * (halfWidthInPixel[index + 1] - halfWidthInPixel[index]);
  else
    halfWidth = halfWidthInPixel[index];

  for (RouteGraphics & graphics : m_routeGraphics)
  {
    dp::UniformValuesStorage uniformStorage;
    glsl::vec4 color = glsl::ToVec4(graphics.m_color);
    uniformStorage.SetFloatValue("u_color", color.r, color.g, color.b, color.a);
    uniformStorage.SetFloatValue("u_halfWidth", halfWidth, halfWidth * screen.GetScale());
    uniformStorage.SetFloatValue("u_clipLength", m_distanceFromBegin);

    ref_ptr<dp::GpuProgram> prg = mng->GetProgram(graphics.m_state.GetProgramIndex());
    prg->Bind();
    dp::ApplyBlending(graphics.m_state, prg);
    dp::ApplyUniforms(commonUniforms, prg);
    dp::ApplyUniforms(uniformStorage, prg);

    graphics.m_buffer->Render();

    // TEST
    double arrowHalfWidth = halfWidth * arrowHeightFactor;
    uniformStorage.SetFloatValue("u_halfWidth", arrowHalfWidth, arrowHalfWidth * screen.GetScale());
    uniformStorage.SetFloatValue("u_textureRect", m_arrowTextureRect.minX(), m_arrowTextureRect.minY(),
                                 m_arrowTextureRect.maxX(), m_arrowTextureRect.maxY());



    m_turnPoints = { 0.0091, 0.0102 };
    float const textureWidth = 2.0 * arrowHalfWidth * arrowAspect;
    vector<ArrowBorders> arrowBorders;
    CalculateArrowBorders(0.001, screen.GetScale(), textureWidth, arrowBorders);

    size_t const elementsCount = 16;
    vector<float> borders(elementsCount, 0.0);
    size_t index = 0;
    for (size_t i = 0; i < arrowBorders.size(); i++)
    {
      borders[index++] = arrowBorders[i].m_startDistance;
      borders[index++] = arrowBorders[i].m_startTexCoord;
      borders[index++] = arrowBorders[i].m_endDistance;
      borders[index++] = arrowBorders[i].m_endTexCoord;

      // fill rests by zeros
      if (i == arrowBorders.size() - 1)
      {
         for (size_t j = index; j < elementsCount; j++)
           borders[j] = 0.0;

         index = elementsCount;
      }

      // render arrow's parts
      if (index == elementsCount)
      {
        index = 0;
        uniformStorage.SetMatrix4x4Value("u_arrowBorders", borders.data());

        ref_ptr<dp::GpuProgram> prgArrow = mng->GetProgram(gpu::ROUTE_ARROW_PROGRAM);
        prgArrow->Bind();
        dp::ApplyState(graphics.m_state, prgArrow);
        dp::ApplyUniforms(commonUniforms, prgArrow);
        dp::ApplyUniforms(uniformStorage, prgArrow);
        graphics.m_buffer->Render();
      }
    }
  }
}

void RouteRenderer::AddRouteRenderBucket(dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket,
                                         dp::Color const & color,  m2::RectF const & arrowTextureRect,
                                         ref_ptr<dp::GpuProgramManager> mng)
{
  m_routeGraphics.push_back(RouteGraphics());
  RouteGraphics & route = m_routeGraphics.back();

  route.m_state = state;
  route.m_color = color;
  route.m_buffer = bucket->MoveBuffer();
  route.m_buffer->Build(mng->GetProgram(route.m_state.GetProgramIndex()));

  m_arrowTextureRect = arrowTextureRect;
}

void RouteRenderer::Clear()
{
  m_routeGraphics.clear();
}

void RouteRenderer::UpdateDistanceFromBegin(double distanceFromBegin)
{
  m_distanceFromBegin = distanceFromBegin;
}

void RouteRenderer::CalculateArrowBorders(double arrowLength, double scale,
                                          float arrowTextureWidth, vector<ArrowBorders> & borders)
{
  if (m_turnPoints.empty())
    return;

  double const halfLen = 0.5 * arrowLength;

  borders.reserve(m_turnPoints.size() * arrowPartsCount);

  // initial filling
  for (size_t i = 0; i < m_turnPoints.size(); i++)
  {
    ArrowBorders arrowBorders;
    arrowBorders.m_groupIndex = (int)i;
    arrowBorders.m_startDistance = m_turnPoints[i] - halfLen;
    arrowBorders.m_endDistance = m_turnPoints[i] + halfLen;
    borders.push_back(arrowBorders);
  }

  // mark groups
  for (size_t i = 0; i < borders.size() - 1; i++)
  {
    if (borders[i].m_endDistance >= borders[i + 1].m_startDistance)
      borders[i + 1].m_groupIndex = borders[i].m_groupIndex;
  }

  // merge groups
  int lastGroup = 0;
  size_t lastGroupIndex = 0;
  for (size_t i = 1; i < borders.size(); i++)
  {
    if (borders[i].m_groupIndex != lastGroup)
    {
      borders[lastGroupIndex].m_endDistance = borders[i - 1].m_endDistance;
      lastGroupIndex = i;
      lastGroup = borders[i].m_groupIndex;
    }
    else
    {
      borders[i].m_groupIndex = -1;
    }
  }
  borders[lastGroupIndex].m_endDistance = borders.back().m_endDistance;

  // clip groups
  auto const iter = remove_if(borders.begin(), borders.end(),
                              [&scale, &arrowTextureWidth](ArrowBorders const & borders)
  {
      if (borders.m_groupIndex == -1)
        return true;

      double const distanceInPixels = (borders.m_endDistance - borders.m_startDistance) * 0.9 / scale;
      return distanceInPixels < (arrowHeadSize + arrowTailSize) * arrowTextureWidth;
  });
  borders.erase(iter, borders.end());

  // divide to parts of arrow
  size_t const bordersSize = borders.size();

  double const glbTextureWidth = arrowTextureWidth * scale;
  for (size_t i = 0; i < bordersSize; i++)
  {
    float const startDistance = borders[i].m_startDistance;
    float const endDistance = borders[i].m_endDistance;

    borders[i].m_endDistance = startDistance + arrowTailSize * glbTextureWidth;
    borders[i].m_startTexCoord = 0.0;
    borders[i].m_endTexCoord = arrowTailSize;

    ArrowBorders arrowHead;
    arrowHead.m_startDistance = endDistance - arrowHeadSize * glbTextureWidth;
    arrowHead.m_endDistance = endDistance;
    arrowHead.m_startTexCoord = 1.0 - arrowHeadSize;
    arrowHead.m_endTexCoord = 1.0;
    borders.push_back(arrowHead);

    ArrowBorders arrowBody;
    arrowBody.m_startDistance = borders[i].m_endDistance;
    arrowBody.m_endDistance = arrowHead.m_startDistance;
    arrowBody.m_startTexCoord = borders[i].m_endTexCoord;
    arrowBody.m_endTexCoord = arrowHead.m_startTexCoord;
    borders.push_back(arrowBody);

    /*for (int j = 1; j <= arrowPartsCount; j++)
    {
      ArrowBorders arrowBorders;
      arrowBorders.m_startDistance = startDistance + arrowTextureParts[j - 1] * len;
      arrowBorders.m_endDistance = startDistance + arrowTextureParts[j] * len;
      arrowBorders.m_startTexCoord = arrowTextureParts[j - 1];
      arrowBorders.m_endTexCoord = arrowTextureParts[j];

      if (j == 1)
        borders[i] = arrowBorders;
      else
        borders.push_back(arrowBorders);
    }*/
  }
}

} // namespace df

