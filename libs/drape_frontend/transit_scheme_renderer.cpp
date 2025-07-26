#include "drape_frontend/transit_scheme_renderer.hpp"

#include "drape_frontend/debug_rect_renderer.hpp"
#include "drape_frontend/postprocess_renderer.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/graphics_context.hpp"
#include "drape/overlay_tree.hpp"

#include <algorithm>

namespace df
{
namespace
{
float CalculateHalfWidth(ScreenBase const & screen)
{
  double zoom = 0.0;
  int index = 0;
  float lerpCoef = 0.0f;
  ExtractZoomFactors(screen, zoom, index, lerpCoef);

  return InterpolateByZoomLevels(index, lerpCoef, kTransitLinesWidthInPixel) *
         static_cast<float>(VisualParams::Instance().GetVisualScale());
}
}  // namespace

bool TransitSchemeRenderer::IsSchemeVisible(int zoomLevel) const
{
  return zoomLevel >= kTransitSchemeMinZoomLevel;
}

bool TransitSchemeRenderer::HasRenderData() const
{
  return !m_linesRenderData.empty() || !m_linesCapsRenderData.empty() || !m_markersRenderData.empty() ||
         !m_textRenderData.empty() || !m_colorSymbolRenderData.empty();
}

void TransitSchemeRenderer::ClearContextDependentResources(ref_ptr<dp::OverlayTree> tree)
{
  if (tree)
  {
    RemoveOverlays(tree, m_textRenderData);
    RemoveOverlays(tree, m_colorSymbolRenderData);
  }

  m_linesRenderData.clear();
  m_linesCapsRenderData.clear();
  m_markersRenderData.clear();
  m_textRenderData.clear();
  m_colorSymbolRenderData.clear();
}

void TransitSchemeRenderer::Clear(MwmSet::MwmId const & mwmId, ref_ptr<dp::OverlayTree> tree)
{
  ClearRenderData(mwmId, nullptr /* tree */, m_linesRenderData);
  ClearRenderData(mwmId, nullptr /* tree */, m_linesCapsRenderData);
  ClearRenderData(mwmId, nullptr /* tree */, m_markersRenderData);
  ClearRenderData(mwmId, tree, m_textRenderData);
  ClearRenderData(mwmId, tree, m_colorSymbolRenderData);
}

void TransitSchemeRenderer::ClearRenderData(MwmSet::MwmId const & mwmId, ref_ptr<dp::OverlayTree> tree,
                                            std::vector<TransitRenderData> & renderData)
{
  auto removePredicate = [&mwmId](TransitRenderData const & data) { return data.m_mwmId == mwmId; };
  ClearRenderData(removePredicate, tree, renderData);
}

void TransitSchemeRenderer::ClearRenderData(TRemovePredicate const & predicate, ref_ptr<dp::OverlayTree> tree,
                                            std::vector<TransitRenderData> & renderData)
{
  if (tree)
  {
    for (auto & data : renderData)
      if (predicate(data))
        data.m_bucket->RemoveOverlayHandles(tree);
  }

  renderData.erase(std::remove_if(renderData.begin(), renderData.end(), predicate), renderData.end());
}

void TransitSchemeRenderer::AddRenderData(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                          ref_ptr<dp::OverlayTree> tree, TransitRenderData && renderData)
{
  switch (renderData.m_type)
  {
  case TransitRenderData::Type::LinesCaps:
    PrepareRenderData(context, mng, tree, m_linesCapsRenderData, std::move(renderData));
    break;
  case TransitRenderData::Type::Lines:
    PrepareRenderData(context, mng, tree, m_linesRenderData, std::move(renderData));
    break;
  case TransitRenderData::Type::Markers:
    PrepareRenderData(context, mng, tree, m_markersRenderData, std::move(renderData));
    break;
  case TransitRenderData::Type::Text:
    PrepareRenderData(context, mng, tree, m_textRenderData, std::move(renderData));
    break;
  case TransitRenderData::Type::Stubs:
    PrepareRenderData(context, mng, tree, m_colorSymbolRenderData, std::move(renderData));
    break;
  }
}

void TransitSchemeRenderer::PrepareRenderData(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                              ref_ptr<dp::OverlayTree> tree,
                                              std::vector<TransitRenderData> & currentRenderData,
                                              TransitRenderData && newRenderData)
{
  // Remove obsolete render data.
  auto const removePredicate = [this, &newRenderData](TransitRenderData const & rd)
  { return rd.m_mwmId == newRenderData.m_mwmId && rd.m_recacheId < m_lastRecacheId; };
  ClearRenderData(removePredicate, tree, currentRenderData);

  m_lastRecacheId = std::max(m_lastRecacheId, newRenderData.m_recacheId);

  // Add new render data.
  auto program = mng->GetProgram(newRenderData.m_state.GetProgram<gpu::Program>());
  program->Bind();
  newRenderData.m_bucket->GetBuffer()->Build(context, program);

  currentRenderData.emplace_back(std::move(newRenderData));
}

void TransitSchemeRenderer::RenderTransit(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                          ScreenBase const & screen, ref_ptr<PostprocessRenderer> postprocessRenderer,
                                          FrameValues const & frameValues, ref_ptr<DebugRectRenderer> debugRectRenderer)
{
  auto const zoomLevel = GetDrawTileScale(screen);
  if (!IsSchemeVisible(zoomLevel) || !HasRenderData())
    return;

  float const pixelHalfWidth = CalculateHalfWidth(screen);

  RenderLinesCaps(context, mng, screen, frameValues, pixelHalfWidth);
  RenderLines(context, mng, screen, frameValues, pixelHalfWidth);
  RenderMarkers(context, mng, screen, frameValues, pixelHalfWidth);
  {
    StencilWriterGuard guard(postprocessRenderer, context);
    RenderText(context, mng, screen, frameValues, debugRectRenderer);
  }
  // Render only for debug purpose.
  // RenderStubs(context, mng, screen, frameValues, debugRectRenderer);
}

void TransitSchemeRenderer::CollectOverlays(ref_ptr<dp::OverlayTree> tree, ScreenBase const & modelView)
{
  CollectOverlays(tree, modelView, m_textRenderData);
  CollectOverlays(tree, modelView, m_colorSymbolRenderData);
}

void TransitSchemeRenderer::CollectOverlays(ref_ptr<dp::OverlayTree> tree, ScreenBase const & modelView,
                                            std::vector<TransitRenderData> & renderData)
{
  for (auto & data : renderData)
    if (tree->IsNeedUpdate())
      data.m_bucket->CollectOverlayHandles(tree);
    else
      data.m_bucket->Update(modelView);
}

void TransitSchemeRenderer::RemoveOverlays(ref_ptr<dp::OverlayTree> tree, std::vector<TransitRenderData> & renderData)
{
  for (auto & data : renderData)
    data.m_bucket->RemoveOverlayHandles(tree);
}

void TransitSchemeRenderer::RenderLinesCaps(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                            ScreenBase const & screen, FrameValues const & frameValues,
                                            float pixelHalfWidth)
{
  context->Clear(dp::ClearBits::DepthBit, dp::kClearBitsStoreAll);
  for (auto & renderData : m_linesCapsRenderData)
  {
    ref_ptr<dp::GpuProgram> program = mng->GetProgram(renderData.m_state.GetProgram<gpu::Program>());
    program->Bind();
    dp::ApplyState(context, program, renderData.m_state);

    gpu::TransitProgramParams params;
    frameValues.SetTo(params);
    math::Matrix<float, 4, 4> mv = screen.GetModelView(renderData.m_pivot, kShapeCoordScalar);
    params.m_modelView = glsl::make_mat4(mv.m_data);
    params.m_lineHalfWidth = pixelHalfWidth;
    params.m_maxRadius = kTransitLineHalfWidth;
    mng->GetParamsSetter()->Apply(context, program, params);

    renderData.m_bucket->Render(context, false /* draw as line */);
  }
}

void TransitSchemeRenderer::RenderLines(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                        ScreenBase const & screen, FrameValues const & frameValues,
                                        float pixelHalfWidth)
{
  for (auto & renderData : m_linesRenderData)
  {
    ref_ptr<dp::GpuProgram> program = mng->GetProgram(renderData.m_state.GetProgram<gpu::Program>());
    program->Bind();
    dp::ApplyState(context, program, renderData.m_state);

    gpu::TransitProgramParams params;
    frameValues.SetTo(params);
    math::Matrix<float, 4, 4> mv = screen.GetModelView(renderData.m_pivot, kShapeCoordScalar);
    params.m_modelView = glsl::make_mat4(mv.m_data);
    params.m_lineHalfWidth = pixelHalfWidth;
    mng->GetParamsSetter()->Apply(context, program, params);

    renderData.m_bucket->Render(context, false /* draw as line */);
  }
}

void TransitSchemeRenderer::RenderMarkers(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                          ScreenBase const & screen, FrameValues const & frameValues,
                                          float pixelHalfWidth)
{
  context->Clear(dp::ClearBits::DepthBit, dp::kClearBitsStoreAll);
  for (auto & renderData : m_markersRenderData)
  {
    auto program = mng->GetProgram(renderData.m_state.GetProgram<gpu::Program>());
    program->Bind();
    dp::ApplyState(context, program, renderData.m_state);

    gpu::TransitProgramParams params;
    frameValues.SetTo(params);
    math::Matrix<float, 4, 4> mv = screen.GetModelView(renderData.m_pivot, kShapeCoordScalar);
    params.m_modelView = glsl::make_mat4(mv.m_data);
    params.m_params = glsl::vec3(static_cast<float>(cos(screen.GetAngle())), static_cast<float>(sin(screen.GetAngle())),
                                 pixelHalfWidth);
    mng->GetParamsSetter()->Apply(context, program, params);

    renderData.m_bucket->Render(context, false /* draw as line */);
  }
}

void TransitSchemeRenderer::RenderText(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                       ScreenBase const & screen, FrameValues const & frameValues,
                                       ref_ptr<DebugRectRenderer> debugRectRenderer)
{
  auto const & glyphParams = df::VisualParams::Instance().GetGlyphVisualParams();
  for (auto & renderData : m_textRenderData)
  {
    ref_ptr<dp::GpuProgram> program = mng->GetProgram(renderData.m_state.GetProgram<gpu::Program>());
    program->Bind();
    dp::ApplyState(context, program, renderData.m_state);

    gpu::MapProgramParams params;
    frameValues.SetTo(params);
    math::Matrix<float, 4, 4> mv = screen.GetModelView(renderData.m_pivot, kShapeCoordScalar);
    params.m_modelView = glsl::make_mat4(mv.m_data);
    params.m_contrastGamma = glsl::vec2(glyphParams.m_outlineContrast, glyphParams.m_outlineGamma);
    params.m_isOutlinePass = 1.0f;
    mng->GetParamsSetter()->Apply(context, program, params);

    renderData.m_bucket->Render(context, false /* draw as line */);

    params.m_contrastGamma = glsl::vec2(glyphParams.m_contrast, glyphParams.m_gamma);
    params.m_isOutlinePass = 0.0f;
    mng->GetParamsSetter()->Apply(context, program, params);

    renderData.m_bucket->Render(context, false /* draw as line */);

    renderData.m_bucket->RenderDebug(context, screen, debugRectRenderer);
  }
}

void TransitSchemeRenderer::RenderStubs(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                        ScreenBase const & screen, FrameValues const & frameValues,
                                        ref_ptr<DebugRectRenderer> debugRectRenderer)
{
  for (auto & renderData : m_colorSymbolRenderData)
  {
    auto program = mng->GetProgram(renderData.m_state.GetProgram<gpu::Program>());
    program->Bind();
    dp::ApplyState(context, program, renderData.m_state);

    gpu::MapProgramParams params;
    frameValues.SetTo(params);
    math::Matrix<float, 4, 4> mv = screen.GetModelView(renderData.m_pivot, kShapeCoordScalar);
    params.m_modelView = glsl::make_mat4(mv.m_data);
    mng->GetParamsSetter()->Apply(context, program, params);

    renderData.m_bucket->Render(context, false /* draw as line */);

    renderData.m_bucket->RenderDebug(context, screen, debugRectRenderer);
  }
}
}  // namespace df
