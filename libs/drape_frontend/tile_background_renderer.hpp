#pragma once

#include "drape_frontend/frame_values.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "shaders/program_manager.hpp"

#include "drape/drape_global.hpp"
#include "drape/pointers.hpp"

#include "geometry/screenbase.hpp"

#include "base/buffer_vector.hpp"

#include <vector>

namespace df
{
class TileBackgroundRenderer final
{
public:
  TileBackgroundRenderer(MapDataProvider::TTileBackgroundReadFn && tileBackgroundReadFn,
                         MapDataProvider::TCancelTileBackgroundReadingFn && cancelTileBackgroundReadingFn);

  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
              int zoomLevel, FrameValues const & frameValues);

  void ClearContextDependentResources();

  void OnUpdateViewport(CoverageResult const & coverage, int currentZoomLevel,
                        buffer_vector<TileKey, 8> const & tilesToDelete);

  void SetBackgroundMode(dp::BackgroundMode mode);
  dp::BackgroundMode GetBackgroundMode() const;

private:
  MapDataProvider::TTileBackgroundReadFn m_tileBackgroundReadFn;
  MapDataProvider::TCancelTileBackgroundReadingFn m_cancelTileBackgroundReadingFn;

  dp::BackgroundMode m_currentMode = dp::BackgroundMode::Default;
};
}  // namespace df
