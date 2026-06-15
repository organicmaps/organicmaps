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

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/buffer_vector.hpp"

#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace df
{
class TileBackgroundRenderer final
{
public:
  TileBackgroundRenderer(MapDataProvider::TTileBackgroundReadFn && tileBackgroundReadFn,
                         MapDataProvider::TCancelTileBackgroundReadingFn && cancelTileBackgroundReadingFn,
                         dp::BackgroundMode currentMode);

  // NOT THREAD SAFE!
  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
              int zoomLevel, FrameValues const & frameValues);

  void ClearContextDependentResources(ref_ptr<dp::GraphicsContext> context);

  void OnUpdateViewport(ref_ptr<dp::GraphicsContext> context, CoverageResult const & coverage, int currentZoomLevel);

  // Registers an image (uploaded by the backend) with the given uid. The image starts unreferenced;
  // it lives in the unreferenced LRU until SetTileBackgroundData binds it to a tile.
  void AssignTileBackgroundImage(ref_ptr<dp::GraphicsContext> context, std::string const & uid,
                                 ref_ptr<dp::TexturePool> texturePool, dp::TexturePool::TextureId textureId,
                                 dp::BackgroundMode mode);

  // Binds a tile to an image with a sub-rect (full image is (0, 0, 1, 1)).
  void SetTileBackgroundData(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                             std::string const & imageUid, m2::RectF const & rect);

  void SetBackgroundMode(ref_ptr<dp::GraphicsContext> context, dp::BackgroundMode mode);
  dp::BackgroundMode GetBackgroundMode() const;

  // True if an image with this uid is already registered (referenced or in the unreferenced LRU).
  // Lets the caller skip re-uploading a duplicate texture that AssignTileBackgroundImage would discard
  // anyway (over-zoom siblings and panning re-requests deliver the same uid repeatedly).
  bool HasImage(std::string const & uid) const { return m_images.find(uid) != m_images.end(); }

private:
  struct ImageInfo
  {
    ref_ptr<dp::TexturePool> m_texturePool;
    dp::TexturePool::TextureId m_textureId{};
    dp::BackgroundMode m_mode = dp::BackgroundMode::Default;
    uint32_t m_refCount = 0;
  };

  struct TileBinding
  {
    std::string m_imageUid;
    m2::RectF m_rect;
  };

  struct DrawEntry
  {
    TileKey m_tileKey;
    ImageInfo const * m_image;
    m2::RectF m_rect;
  };

  // Decrements refcount; if it reaches 0, pushes uid to the unreferenced LRU.
  void ReleaseImageRef(ref_ptr<dp::GraphicsContext> context, std::string const & uid);
  // Increments refcount; if it was 0, removes uid from the unreferenced LRU.
  void AcquireImageRef(std::string const & uid);
  // Drops the per-tile binding (if any) and decrements its image refcount.
  void ReleaseTileBinding(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey);

  MapDataProvider::TTileBackgroundReadFn m_tileBackgroundReadFn;
  MapDataProvider::TCancelTileBackgroundReadingFn m_cancelTileBackgroundReadingFn;

  dp::BackgroundMode m_currentMode = dp::BackgroundMode::Default;

  std::unordered_set<TileKey> m_awaitingTiles;
  std::unordered_map<TileKey, TileBinding> m_tiles;

  std::unordered_map<std::string, ImageInfo> m_images;
  // LRU of image uids whose refCount == 0 (oldest at front). Bounded by kMaxUnreferencedImages.
  std::list<std::string> m_unreferencedLRU;

  CoverageResult m_lastCoverage;
  int m_lastCurrentZoomLevel = 0;

  dp::RenderState m_state;
  dp::RenderState m_stateArray;
  gpu::TileBackgroundProgramParams m_programParams{};

  // Per-frame scratch buffer for Render(); a member to reuse the allocation between frames.
  std::vector<DrawEntry> m_sortedTiles;

  drape_ptr<dp::Instancing> m_instancing;
};
}  // namespace df
