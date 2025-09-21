#include "drape_frontend/tile_background_renderer.hpp"

#include "base/logging.hpp"

#include <algorithm>

namespace df
{
TileBackgroundRenderer::TileBackgroundRenderer(
    MapDataProvider::TTileBackgroundReadFn && tileBackgroundReadFn,
    MapDataProvider::TCancelTileBackgroundReadingFn && cancelTileBackgroundReadingFn)
  : m_tileBackgroundReadFn(std::move(tileBackgroundReadFn))
  , m_cancelTileBackgroundReadingFn(std::move(cancelTileBackgroundReadingFn))
{
  CHECK(m_tileBackgroundReadFn != nullptr, ());
  CHECK(m_cancelTileBackgroundReadingFn != nullptr, ());
}

void TileBackgroundRenderer::OnUpdateViewport(CoverageResult const & coverage, int currentZoomLevel,
                                              buffer_vector<TileKey, 8> const & tilesToDelete)
{}

void TileBackgroundRenderer::Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                    ScreenBase const & screen, int zoomLevel, FrameValues const & frameValues)
{
  if (m_currentMode == dp::BackgroundMode::Default)
    return;

  // TODO
}

void TileBackgroundRenderer::ClearContextDependentResources() {}

void TileBackgroundRenderer::SetBackgroundMode(dp::BackgroundMode mode)
{
  if (m_currentMode != mode)
  {
    m_currentMode = mode;
    // TODO: Update render data when background mode changes
    // - May need to invalidate existing render data
    // - Trigger re-rendering with new mode
  }
}

dp::BackgroundMode TileBackgroundRenderer::GetBackgroundMode() const
{
  return m_currentMode;
}
}  // namespace df
