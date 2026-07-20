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

#include <list>
#include <string>
#include <unordered_map>
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
              FrameValues const & frameValues);

  void ClearContextDependentResources(ref_ptr<dp::GraphicsContext> context);

  void OnUpdateViewport(ref_ptr<dp::GraphicsContext> context, CoverageResult coverage, uint8_t currentZoomLevel);

  // Registers an uploaded image and keeps it alive until the matching request is bound or cancelled.
  void AssignTileBackgroundImage(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey, std::string const & uid,
                                 ref_ptr<dp::TexturePool> texturePool, dp::TexturePool::TextureId textureId,
                                 dp::BackgroundMode mode);

  // Binds the image sub-rect. An empty uid marks a failed read until coverage exit or layer reset.
  void SetTileBackgroundData(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                             std::string const & imageUid, m2::RectF const & rect);

  void SetBackgroundMode(ref_ptr<dp::GraphicsContext> context, dp::BackgroundMode mode);
  dp::BackgroundMode GetBackgroundMode() const;

  // Registered images can skip duplicate uploads, including over-zoom siblings sharing an ancestor.
  bool HasImage(std::string const & uid) const { return m_images.find(uid) != m_images.end(); }

  struct TileRect
  {
    TileKey m_tileKey;
    m2::RectF m_textureRect;
  };

  // Appends the parts not covered by current-level tiles, preserving the image's UV sub-rect.
  // Splitting coarse fallback tiles prevents transparent imagery from blending twice. Public for tests.
  void GetTileRects(TileKey const & tileKey, m2::RectF const & textureRect, std::vector<TileRect> & rects) const;

private:
  struct ImageInfo
  {
    ref_ptr<dp::TexturePool> m_texturePool;
    dp::TexturePool::TextureId m_textureId{};
    // Live tile bindings and uploads awaiting their matching bind message.
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
  // Pending upload/bind pairs hold image references separately from this bounded cache.
  void TrimUnreferencedImages(ref_ptr<dp::GraphicsContext> context);
  // Fraction of |viewportRect| covered by the bindings of |zoomLevel| (0 for TileKey::kNoZoom).
  // Fallback selection compares this value because binding count does not represent screen area.
  double CoveredFraction(uint8_t zoomLevel, m2::RectD const & viewportRect) const;
  // Releases the fallback once all accepted current-level reads succeed.
  void RetireFallbackIfReady(ref_ptr<dp::GraphicsContext> context);

  MapDataProvider::TTileBackgroundReadFn m_tileBackgroundReadFn;
  MapDataProvider::TCancelTileBackgroundReadingFn m_cancelTileBackgroundReadingFn;

  dp::BackgroundMode m_currentMode = dp::BackgroundMode::Default;

  struct TileRequest
  {
    uint64_t m_generation;
    // Image reference held between this request's upload and bind messages; empty before upload.
    std::string m_imageUid;
  };

  // Pending or failed requests; failures remain until coverage exit or layer reset.
  // Coordinate keys ignore m_generation, so check it separately.
  std::unordered_map<TileKey, TileRequest> m_tileRequests;
  uint64_t m_nextRequestGeneration = 0;
  std::unordered_map<TileKey, TileBinding> m_tiles;

  std::unordered_map<std::string, ImageInfo> m_images;
  // LRU of image uids whose refCount == 0 (oldest at front). Bounded by kMaxUnreferencedImages.
  std::list<std::string> m_unreferencedLRU;

  CoverageResult m_lastCoverage;
  uint8_t m_lastCurrentZoomLevel = TileKey::kNoZoom;
  // Covers pending or failed current-level gaps. TileKey::kNoZoom means no fallback is retained.
  uint8_t m_fallbackZoomLevel = TileKey::kNoZoom;

  dp::RenderState m_state;
  dp::RenderState m_stateArray;
  gpu::TileBackgroundProgramParams m_programParams{};

  // Per-frame scratch buffer for Render(); a member to reuse the allocation between frames.
  std::vector<DrawEntry> m_sortedTiles;
  std::vector<TileRect> m_tileRects;

  drape_ptr<dp::Instancing> m_instancing;
};
}  // namespace df
