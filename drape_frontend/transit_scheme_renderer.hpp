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
class PostprocessRenderer;
class OverlayTree;

class TransitSchemeRenderer
{
public:
  void AddRenderData(ref_ptr<gpu::ProgramManager> mng, ref_ptr<dp::OverlayTree> tree, TransitRenderData && renderData);

  bool IsSchemeVisible(int zoomLevel) const;

  void RenderTransit(ScreenBase const & screen, ref_ptr<gpu::ProgramManager> mng,
                     ref_ptr<PostprocessRenderer> postprocessRenderer,
                     ref_ptr<dp::GraphicsContext> context,
                     FrameValues const & frameValues);

  void CollectOverlays(ref_ptr<dp::OverlayTree> tree, ScreenBase const & modelView);


  void ClearGLDependentResources(ref_ptr<dp::OverlayTree> tree);

  void Clear(MwmSet::MwmId const & mwmId, ref_ptr<dp::OverlayTree> tree);

private:
  void PrepareRenderData(ref_ptr<gpu::ProgramManager> mng, ref_ptr<dp::OverlayTree> tree,
                         std::vector<TransitRenderData> & currentRenderData,
                         TransitRenderData && newRenderData);
  void ClearRenderData(MwmSet::MwmId const & mwmId, ref_ptr<dp::OverlayTree> tree,
                       std::vector<TransitRenderData> & renderData);

  using TRemovePredicate = std::function<bool(TransitRenderData const &)>;
  void ClearRenderData(TRemovePredicate const & predicate, ref_ptr<dp::OverlayTree> tree,
                       std::vector<TransitRenderData> & renderData);

  void RemoveOverlays(ref_ptr<dp::OverlayTree> tree, std::vector<TransitRenderData> & renderData);
  void CollectOverlays(ref_ptr<dp::OverlayTree> tree, ScreenBase const & modelView,
                       std::vector<TransitRenderData> & renderData);

  void RenderLines(ScreenBase const & screen, ref_ptr<gpu::ProgramManager> mng,
                   FrameValues const & frameValues, float pixelHalfWidth);
  void RenderLinesCaps(ScreenBase const & screen, ref_ptr<gpu::ProgramManager> mng,
                       ref_ptr<dp::GraphicsContext> context, FrameValues const & frameValues, float pixelHalfWidth);
  void RenderMarkers(ScreenBase const & screen, ref_ptr<gpu::ProgramManager> mng,
                     ref_ptr<dp::GraphicsContext> context, FrameValues const & frameValues, float pixelHalfWidth);
  void RenderText(ScreenBase const & screen, ref_ptr<gpu::ProgramManager> mng,
                  FrameValues const & frameValues);
  void RenderStubs(ScreenBase const & screen, ref_ptr<gpu::ProgramManager> mng,
                   FrameValues const & frameValues);

  bool HasRenderData() const;

  uint32_t m_lastRecacheId = 0;
  std::vector<TransitRenderData> m_linesRenderData;
  std::vector<TransitRenderData> m_linesCapsRenderData;
  std::vector<TransitRenderData> m_markersRenderData;
  std::vector<TransitRenderData> m_textRenderData;
  std::vector<TransitRenderData> m_colorSymbolRenderData;
};
}  // namespace df
