#pragma once

#include "drape_frontend/frame_values.hpp"
#include "drape_frontend/transit_scheme_builder.hpp"

#include "shaders/program_manager.hpp"

#include "drape/pointers.hpp"

#include "geometry/screenbase.hpp"

#include <functional>

namespace dp
{
class GraphicsContext;
}  // namespace dp

namespace df
{
class DebugRectRenderer;
class OverlayTree;
class PostprocessRenderer;

class TransitSchemeRenderer
{
public:
  void AddRenderData(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                     ref_ptr<dp::OverlayTree> tree, TransitRenderData && renderData);

  bool IsSchemeVisible(int zoomLevel) const;

  void RenderTransit(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
                     ref_ptr<PostprocessRenderer> postprocessRenderer, FrameValues const & frameValues,
                     ref_ptr<DebugRectRenderer> debugRectRenderer);

  void CollectOverlays(ref_ptr<dp::OverlayTree> tree, ScreenBase const & modelView);

  void ClearContextDependentResources(ref_ptr<dp::OverlayTree> tree);

  void Clear(MwmSet::MwmId const & mwmId, ref_ptr<dp::OverlayTree> tree);

private:
  void PrepareRenderData(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                         ref_ptr<dp::OverlayTree> tree, std::vector<TransitRenderData> & currentRenderData,
                         TransitRenderData && newRenderData);
  void ClearRenderData(MwmSet::MwmId const & mwmId, ref_ptr<dp::OverlayTree> tree,
                       std::vector<TransitRenderData> & renderData);

  using TRemovePredicate = std::function<bool(TransitRenderData const &)>;
  void ClearRenderData(TRemovePredicate const & predicate, ref_ptr<dp::OverlayTree> tree,
                       std::vector<TransitRenderData> & renderData);

  void RemoveOverlays(ref_ptr<dp::OverlayTree> tree, std::vector<TransitRenderData> & renderData);
  void CollectOverlays(ref_ptr<dp::OverlayTree> tree, ScreenBase const & modelView,
                       std::vector<TransitRenderData> & renderData);

  void RenderLines(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
                   FrameValues const & frameValues, float pixelHalfWidth);
  void RenderLinesCaps(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                       ScreenBase const & screen, FrameValues const & frameValues, float pixelHalfWidth);
  void RenderMarkers(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
                     FrameValues const & frameValues, float pixelHalfWidth);
  void RenderText(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
                  FrameValues const & frameValues, ref_ptr<DebugRectRenderer> debugRectRenderer);
  void RenderStubs(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
                   FrameValues const & frameValues, ref_ptr<DebugRectRenderer> debugRectRenderer);

  bool HasRenderData() const;

  uint32_t m_lastRecacheId = 0;
  std::vector<TransitRenderData> m_linesRenderData;
  std::vector<TransitRenderData> m_linesCapsRenderData;
  std::vector<TransitRenderData> m_markersRenderData;
  std::vector<TransitRenderData> m_textRenderData;
  std::vector<TransitRenderData> m_colorSymbolRenderData;
};
}  // namespace df
