#pragma once

#include "drape_frontend/frame_values.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "shaders/program_manager.hpp"

#include "drape/drape_global.hpp"
#include "drape/instancing.hpp"
#include "drape/pointers.hpp"
#include "drape/render_state.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/screenbase.hpp"

#include "base/buffer_vector.hpp"

#include <list>
#include <optional>
#include <unordered_map>
#include <unordered_set>
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

  void ClearContextDependentResources(ref_ptr<dp::GraphicsContext> context);

  void OnUpdateViewport(ref_ptr<dp::GraphicsContext> context, CoverageResult const & coverage, int currentZoomLevel,
                        buffer_vector<TileKey, 8> const & tilesToDelete);

  void AssignTileBackgroundTexture(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                                   ref_ptr<dp::TexturePool> texturePool, dp::TexturePool::TextureId textureId,
                                   dp::BackgroundMode mode);

  void SetBackgroundMode(ref_ptr<dp::GraphicsContext> context, dp::BackgroundMode mode);
  dp::BackgroundMode GetBackgroundMode() const;

private:
  struct TextureInfo
  {
    ref_ptr<dp::TexturePool> m_texturePool;
    dp::TexturePool::TextureId m_textureId{};
  };
  void RemoveTexture(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey, TextureInfo const & info);
  std::optional<TextureInfo> RestoreRemovedTexture(TileKey const & tileKey);

  MapDataProvider::TTileBackgroundReadFn m_tileBackgroundReadFn;
  MapDataProvider::TCancelTileBackgroundReadingFn m_cancelTileBackgroundReadingFn;

  dp::BackgroundMode m_currentMode = dp::BackgroundMode::Default;

  std::unordered_set<TileKey> m_awaitingTiles;
  std::unordered_map<TileKey, TextureInfo> m_tileTextures;

  using RemoveTexturesList = std::list<std::pair<TileKey, TextureInfo>>;
  RemoveTexturesList m_removedTextures;
  std::unordered_map<TileKey, RemoveTexturesList::iterator> m_removedTexturesCache;

  CoverageResult m_lastCoverage;
  int m_lastCurrentZoomLevel = 0;

  dp::RenderState m_state;
  dp::RenderState m_stateArray;
  gpu::TileBackgroundProgramParams m_programParams{};

  drape_ptr<dp::Instancing> m_instancing;
};
}  // namespace df
