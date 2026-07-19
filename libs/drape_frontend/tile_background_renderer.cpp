#include "drape_frontend/tile_background_renderer.hpp"

#include "drape_frontend/render_state_extension.hpp"

#include "base/logging.hpp"

#include <algorithm>

namespace df
{
namespace
{
// Max number of refcount==0 images kept alive in the LRU before their texture slots are recycled.
constexpr size_t kMaxUnreferencedImages = 16;

// Background tiles live on their own grid at the true zoom, not on the vector data grid, so their
// rects must never be clipped by the max data zoom — doing so would compute a data-zoom-sized rect
// from true-zoom tile coords.
constexpr bool kClipByDataMaxZoom = false;

// A tile is "wanted" iff it is in the current coverage rect at the current zoom. Background tiles
// are keyed solely by (x, y, zoom) — there is no generation — so the viewport coverage is the single
// source of truth; we don't consult the vector pipeline's |tilesToDelete|.
bool IsInCoverage(TileKey const & tileKey, CoverageResult const & coverage, uint8_t zoomLevel)
{
  return tileKey.m_zoomLevel == zoomLevel && tileKey.m_x >= coverage.m_minTileX &&
         tileKey.m_x < coverage.m_maxTileX && tileKey.m_y >= coverage.m_minTileY && tileKey.m_y < coverage.m_maxTileY;
}

// Global rect spanned by the coverage at the given zoom. Retained tiles come from another zoom
// level, so they can only be tested against the viewport geometrically.
m2::RectD CalcCoverageRect(CoverageResult const & coverage, uint8_t zoomLevel)
{
  if (zoomLevel == TileKey::kNoZoom || coverage.m_minTileX >= coverage.m_maxTileX ||
      coverage.m_minTileY >= coverage.m_maxTileY)
    return {};

  auto const minRect = TileKey(coverage.m_minTileX, coverage.m_minTileY, zoomLevel).GetGlobalRect(kClipByDataMaxZoom);
  auto const maxRect =
      TileKey(coverage.m_maxTileX - 1, coverage.m_maxTileY - 1, zoomLevel).GetGlobalRect(kClipByDataMaxZoom);
  return m2::RectD(minRect.minX(), minRect.minY(), maxRect.maxX(), maxRect.maxY());
}
}  // namespace

TileBackgroundRenderer::TileBackgroundRenderer(
    MapDataProvider::TTileBackgroundReadFn && tileBackgroundReadFn,
    MapDataProvider::TCancelTileBackgroundReadingFn && cancelTileBackgroundReadingFn, dp::BackgroundMode currentMode)
  : m_tileBackgroundReadFn(std::move(tileBackgroundReadFn))
  , m_cancelTileBackgroundReadingFn(std::move(cancelTileBackgroundReadingFn))
  , m_currentMode(currentMode)
  , m_state(CreateRenderState(gpu::Program::TileBackground, DepthLayer::GeometryLayer))
  , m_stateArray(CreateRenderState(gpu::Program::TileBackgroundArray, DepthLayer::GeometryLayer))
  , m_instancing(std::make_unique<dp::Instancing>())
{
  CHECK(m_tileBackgroundReadFn != nullptr, ());
  CHECK(m_cancelTileBackgroundReadingFn != nullptr, ());

  // Blending is enabled so raster tiles with transparent areas (e.g. partial/overview tiles with
  // a tRNS palette) let the map below show through instead of compositing as opaque black.
  m_state.SetBlending(dp::Blending(true /* isEnabled */));
  m_state.SetDepthTestEnabled(false);

  m_stateArray.SetBlending(dp::Blending(true /* isEnabled */));
  m_stateArray.SetDepthTestEnabled(false);
}

void TileBackgroundRenderer::OnUpdateViewport(ref_ptr<dp::GraphicsContext> context, CoverageResult const & coverage,
                                              uint8_t currentZoomLevel)
{
  uint8_t const prevZoomLevel = m_lastCurrentZoomLevel;
  m_lastCoverage = coverage;
  m_lastCurrentZoomLevel = currentZoomLevel;
  if (m_currentMode == dp::BackgroundMode::Default)
    return;

  if (context == nullptr)
    return;

  // Refilling a zoom level is asynchronous (read -> decode -> upload -> bind), so dropping the level
  // we are leaving right here would blank the layer for the whole round trip. Keep it as a fallback
  // underneath the new level instead, and retire it only once the new one has finished loading.
  //
  // The level we are leaving takes over as the fallback only if it covers at least as much of the
  // new viewport as the incumbent does. A pinch crossing several levels abandons each one part
  // loaded, and a level holding a tile or two would otherwise displace a complete one and uncover
  // the rest of the screen. Comparing coverage rather than demanding a complete level also keeps
  // the best available layer when nothing covers the viewport fully — after a pan with reads still
  // in flight, or a zoom out onto ground no level has ever loaded.
  if (currentZoomLevel != prevZoomLevel)
  {
    auto const viewportRect = CalcCoverageRect(coverage, currentZoomLevel);
    if (CoveredFraction(prevZoomLevel, viewportRect) >= CoveredFraction(m_fallbackZoomLevel, viewportRect))
      m_fallbackZoomLevel = prevZoomLevel;
    if (m_fallbackZoomLevel == currentZoomLevel)
      m_fallbackZoomLevel = TileKey::kNoZoom;  // zoomed back onto it: it is the current level again
  }

  // Keep background state tied to the current viewport. Sweep both awaiting reads and live bindings
  // so tiles that scrolled out of the coverage are cancelled / released (their image refcounts fall
  // back into the unreferenced LRU), regardless of how the viewport changed.
  for (auto it = m_awaitingTiles.begin(); it != m_awaitingTiles.end();)
  {
    if (IsInCoverage(*it, coverage, currentZoomLevel))
    {
      ++it;
      continue;
    }

    TileKey const tileKey = *it;
    it = m_awaitingTiles.erase(it);
    m_cancelTileBackgroundReadingFn(tileKey, m_currentMode);
  }

  // Bindings are kept if they belong to the current level and coverage, or to the single retained
  // fallback level and still overlap the viewport. Everything else (older levels, tiles scrolled
  // away) is released, which bounds the layer to at most two levels at any time.
  m2::RectD const coverageRect =
      m_fallbackZoomLevel != TileKey::kNoZoom ? CalcCoverageRect(coverage, currentZoomLevel) : m2::RectD();
  for (auto it = m_tiles.begin(); it != m_tiles.end();)
  {
    TileKey const & tileKey = it->first;
    if (IsInCoverage(tileKey, coverage, currentZoomLevel) ||
        (tileKey.m_zoomLevel == m_fallbackZoomLevel &&
         tileKey.GetGlobalRect(kClipByDataMaxZoom).IsIntersect(coverageRect)))
    {
      ++it;
      continue;
    }

    ReleaseImageRef(context, it->second.m_imageUid);
    it = m_tiles.erase(it);
  }

  // Request tile background reading for new tiles in the coverage area.
  // The image cache (m_images / unreferenced LRU) is keyed by image uid, which the caller controls,
  // so we always defer the cache check to the read function side: if the image is still alive, the
  // backend's AssignTileBackgroundImage path will dedupe it.
  //
  // Mark a tile as awaiting only after the provider accepts the request. Providers may decline
  // synchronously (unsupported zoom, outside coverage, or failed task posting). If such tiles were
  // inserted into m_awaitingTiles, they would stay there forever because no result or cancellation
  // callback will arrive to erase them.
  for (int x = coverage.m_minTileX; x < coverage.m_maxTileX; ++x)
  {
    for (int y = coverage.m_minTileY; y < coverage.m_maxTileY; ++y)
    {
      TileKey const key(x, y, currentZoomLevel);
      if (m_tiles.count(key) == 0 && m_awaitingTiles.count(key) == 0 && m_tileBackgroundReadFn(key, m_currentMode))
        m_awaitingTiles.insert(key);
    }
  }

  // Nothing was requested (all bound already, or the provider declined the whole viewport) — then
  // the current level is as complete as it will get and the fallback has nothing left to cover.
  RetireFallbackIfReady(context);
}

double TileBackgroundRenderer::CoveredFraction(uint8_t zoomLevel, m2::RectD const & viewportRect) const
{
  double const viewportArea = viewportRect.SizeX() * viewportRect.SizeY();
  if (zoomLevel == TileKey::kNoZoom || viewportArea <= 0.0)
    return 0.0;

  // Tiles of one level are a non-overlapping grid, so the covered area is just the sum of their
  // intersections with the viewport. Walking the bindings (a screenful) rather than the viewport's
  // tile range at |zoomLevel| matters: that range grows 4x per level of distance, so a large zoom
  // jump would enumerate millions of tiles to answer the same question.
  double covered = 0.0;
  for (auto const & tile : m_tiles)
  {
    if (tile.first.m_zoomLevel != zoomLevel)
      continue;

    auto const r = tile.first.GetGlobalRect(kClipByDataMaxZoom);
    double const w = std::min(r.maxX(), viewportRect.maxX()) - std::max(r.minX(), viewportRect.minX());
    double const h = std::min(r.maxY(), viewportRect.maxY()) - std::max(r.minY(), viewportRect.minY());
    if (w > 0.0 && h > 0.0)
      covered += w * h;
  }
  return covered / viewportArea;
}

bool TileBackgroundRenderer::IsCoveredByCurrentZoom(TileKey const & tileKey) const
{
  int const zoomDiff = static_cast<int>(m_lastCurrentZoomLevel) - static_cast<int>(tileKey.m_zoomLevel);
  if (zoomDiff == 0)
    return false;

  if (zoomDiff < 0)
  {
    // The tile is finer than the current level: hidden iff its ancestor cell is bound.
    // Right shift of a negative coordinate is arithmetic (floor), which matches the tile grid.
    int const k = -zoomDiff;
    return m_tiles.count(TileKey(tileKey.m_x >> k, tileKey.m_y >> k, m_lastCurrentZoomLevel)) > 0;
  }

  // The tile is coarser than the current level: every cell inside both the tile and the coverage
  // must be bound. Cells outside the coverage lie beyond the inflated viewport and cannot be seen,
  // so they are not required; the iteration is thereby bounded by the coverage size (a screenful)
  // no matter how far apart the zoom levels are.
  int const cells = 1 << zoomDiff;
  int const minX = std::max(tileKey.m_x * cells, m_lastCoverage.m_minTileX);
  int const maxX = std::min((tileKey.m_x + 1) * cells, m_lastCoverage.m_maxTileX);
  int const minY = std::max(tileKey.m_y * cells, m_lastCoverage.m_minTileY);
  int const maxY = std::min((tileKey.m_y + 1) * cells, m_lastCoverage.m_maxTileY);
  for (int x = minX; x < maxX; ++x)
    for (int y = minY; y < maxY; ++y)
      if (m_tiles.count(TileKey(x, y, m_lastCurrentZoomLevel)) == 0)
        return false;
  return true;
}

void TileBackgroundRenderer::RetireFallbackIfReady(ref_ptr<dp::GraphicsContext> context)
{
  // The awaiting set contains only current-level coverage tiles (the viewport sweep cancels the
  // rest). The fallback retires after the current coverage has no unresolved request.
  if (m_fallbackZoomLevel == TileKey::kNoZoom || !m_awaitingTiles.empty())
    return;

  for (auto it = m_tiles.begin(); it != m_tiles.end();)
  {
    if (it->first.m_zoomLevel == m_fallbackZoomLevel)
    {
      ReleaseImageRef(context, it->second.m_imageUid);
      it = m_tiles.erase(it);
    }
    else
    {
      ++it;
    }
  }
  m_fallbackZoomLevel = TileKey::kNoZoom;
}

void TileBackgroundRenderer::AssignTileBackgroundImage(ref_ptr<dp::GraphicsContext> context, std::string const & uid,
                                                       ref_ptr<dp::TexturePool> texturePool,
                                                       dp::TexturePool::TextureId textureId, dp::BackgroundMode mode)
{
  if (context == nullptr)
    return;

  // Mode mismatch — drop the texture immediately.
  if (mode != m_currentMode)
  {
    texturePool->ReleaseTexture(context, textureId);
    return;
  }

  // Dedupe: if the image is already registered (live or in the LRU), discard the redundant upload.
  if (m_images.find(uid) != m_images.end())
  {
    texturePool->ReleaseTexture(context, textureId);
    return;
  }

  m_images.emplace(uid, ImageInfo{texturePool, textureId, mode, 0});
  m_unreferencedLRU.push_back(uid);

  // Recycle the oldest unreferenced image if the LRU is over budget.
  while (m_unreferencedLRU.size() > kMaxUnreferencedImages)
  {
    auto const & oldest = m_unreferencedLRU.front();
    auto it = m_images.find(oldest);
    if (it != m_images.end())
    {
      it->second.m_texturePool->ReleaseTexture(context, it->second.m_textureId);
      m_images.erase(it);
    }
    m_unreferencedLRU.pop_front();
  }
}

void TileBackgroundRenderer::SetTileBackgroundData(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                                                   std::string const & imageUid, m2::RectF const & rect)
{
  if (context == nullptr)
    return;

  m_awaitingTiles.erase(tileKey);

  // An empty uid is a terminal read result (HTTP error, timeout, undecodable content). The tile
  // stays unbound and becomes requestable on the next viewport update.
  if (imageUid.empty())
  {
    RetireFallbackIfReady(context);
    return;
  }

  // The provider is asynchronous: a placeholder/final result may arrive after the tile was swept
  // from the viewport. Use the same visible-tile predicate as the viewport update (it also covers
  // the zoom check) so clipped tiles are not resurrected after cancellation.
  if (!IsInCoverage(tileKey, m_lastCoverage, m_lastCurrentZoomLevel))
    return;

  auto imageIt = m_images.find(imageUid);
  if (imageIt == m_images.end())
  {
    LOG(LWARNING, ("Unknown image uid", imageUid, "for tile", tileKey.Coord2String()));
    return;
  }

  // Image must match the current background mode.
  if (imageIt->second.m_mode != m_currentMode)
    return;

  // If the tile was already bound, release the previous image first.
  auto tileIt = m_tiles.find(tileKey);
  if (tileIt != m_tiles.end())
  {
    if (tileIt->second.m_imageUid == imageUid)
    {
      tileIt->second.m_rect = rect;
      return;
    }
    ReleaseImageRef(context, tileIt->second.m_imageUid);
  }

  m_tiles[tileKey] = TileBinding{imageUid, rect};
  AcquireImageRef(imageUid);

  // Keep the fallback until the current level has resolved every requested tile, so a partially
  // delivered level never uncovers the rest of the viewport.
  RetireFallbackIfReady(context);
}

void TileBackgroundRenderer::AcquireImageRef(std::string const & uid)
{
  auto it = m_images.find(uid);
  if (it == m_images.end())
    return;

  if (it->second.m_refCount == 0)
    m_unreferencedLRU.remove(uid);

  ++it->second.m_refCount;
}

void TileBackgroundRenderer::ReleaseImageRef(ref_ptr<dp::GraphicsContext> context, std::string const & uid)
{
  auto it = m_images.find(uid);
  if (it == m_images.end())
    return;

  CHECK_GREATER(it->second.m_refCount, 0, ());
  if (--it->second.m_refCount == 0)
  {
    m_unreferencedLRU.push_back(uid);
    while (m_unreferencedLRU.size() > kMaxUnreferencedImages)
    {
      auto const & oldest = m_unreferencedLRU.front();
      auto oldestIt = m_images.find(oldest);
      if (oldestIt != m_images.end())
      {
        oldestIt->second.m_texturePool->ReleaseTexture(context, oldestIt->second.m_textureId);
        m_images.erase(oldestIt);
      }
      m_unreferencedLRU.pop_front();
    }
  }
}

void TileBackgroundRenderer::Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                    ScreenBase const & screen, FrameValues const & frameValues)
{
  if (m_currentMode == dp::BackgroundMode::Default)
    return;

  // Render tiles relative to the screen center.
  frameValues.SetTo(m_programParams);
  auto const pivot = screen.GlobalRect().Center();
  math::Matrix<float, 4, 4> const mv = screen.GetModelView(pivot, 1.0f);
  m_programParams.m_modelView = glsl::make_mat4(mv.m_data);

  // Sort tiles by texture pointer to minimize texture switches.
  m_sortedTiles.clear();
  m_sortedTiles.reserve(m_tiles.size());
  for (auto const & [tileKey, binding] : m_tiles)
  {
    if (!screen.ClipRect().IsIntersect(tileKey.GetGlobalRect(kClipByDataMaxZoom)))
      continue;
    // A fallback tile completely hidden behind the current level must not be drawn: partially
    // transparent imagery would composite twice (the map below would darken), and opaque imagery
    // is pure overdraw. Partially covered fallback tiles still draw beneath the current level.
    if (tileKey.m_zoomLevel != m_lastCurrentZoomLevel && IsCoveredByCurrentZoom(tileKey))
      continue;
    auto imgIt = m_images.find(binding.m_imageUid);
    if (imgIt == m_images.end())
      continue;
    m_sortedTiles.push_back({tileKey, &imgIt->second, binding.m_rect});
  }
  if (m_sortedTiles.empty())
    return;

  std::sort(m_sortedTiles.begin(), m_sortedTiles.end(), [this](DrawEntry const & lhs, DrawEntry const & rhs)
  {
    // Depth testing is off, so draw order is what composites the levels: the retained fallback must
    // go underneath the current level, which is authoritative. Ordering by texture alone would let
    // stale tiles land on top of the tiles that replace them.
    bool const lhsCurrent = lhs.m_tileKey.m_zoomLevel == m_lastCurrentZoomLevel;
    bool const rhsCurrent = rhs.m_tileKey.m_zoomLevel == m_lastCurrentZoomLevel;
    if (lhsCurrent != rhsCurrent)
      return rhsCurrent;

    auto const lhsTex = lhs.m_image->m_texturePool->GetTexture(lhs.m_image->m_textureId);
    auto const rhsTex = rhs.m_image->m_texturePool->GetTexture(rhs.m_image->m_textureId);
    if (lhsTex == rhsTex)
      return lhs.m_tileKey < rhs.m_tileKey;
    return lhsTex < rhsTex;
  });

  // Render tiles in batches with the same texture.
  uint32_t instanceIndex = 0;
  ref_ptr<dp::GpuProgram> prevProgram = nullptr;
  for (size_t i = 0; i < m_sortedTiles.size(); ++i)
  {
    auto const & entry = m_sortedTiles[i];
    auto const r = entry.m_tileKey.GetGlobalRect(kClipByDataMaxZoom);
    auto const minR = (m2::PointD(r.minX(), r.minY()) - pivot);
    auto const maxR = (m2::PointD(r.maxX(), r.maxY()) - pivot);
    m_programParams.m_tileCoordsMinMax[instanceIndex] = glsl::vec4(
        static_cast<float>(minR.x), static_cast<float>(minR.y), static_cast<float>(maxR.x), static_cast<float>(maxR.y));
    m_programParams.m_textureRectMinMax[instanceIndex] =
        glsl::vec4(entry.m_rect.minX(), entry.m_rect.minY(), entry.m_rect.maxX(), entry.m_rect.maxY());
    m_programParams.m_textureIndex[instanceIndex] = static_cast<int>(entry.m_image->m_textureId);

    auto const tex = entry.m_image->m_texturePool->GetTexture(entry.m_image->m_textureId);
    // Flush the accumulated batch when it is full, this is the last tile, or the next tile uses a different texture.
    bool flushBatch = instanceIndex + 1 == gpu::kTileBackgroundMaxCount || i + 1 == m_sortedTiles.size();
    if (!flushBatch)
      flushBatch =
          tex != m_sortedTiles[i + 1].m_image->m_texturePool->GetTexture(m_sortedTiles[i + 1].m_image->m_textureId);
    if (flushBatch)
    {
      auto & state = entry.m_image->m_texturePool->IsHardwareTexture2dArrayUsed() ? m_stateArray : m_state;

      state.SetColorTexture(tex);

      auto program = mng->GetProgram(state.GetProgram<gpu::Program>());
      if (prevProgram != program)
      {
        context->SetCullingEnabled(false);
        program->Bind();
        prevProgram = program;
      }
      dp::ApplyState(context, program, state);
      mng->GetParamsSetter()->Apply(context, program, m_programParams);

      m_instancing->DrawInstancedTriangleStrip(context, instanceIndex + 1, 4);

      // Restart filling from the beginning
      instanceIndex = 0;
    }
    else
    {
      ++instanceIndex;
    }
  }

  if (prevProgram != nullptr)
  {
    prevProgram->Unbind();
    context->SetCullingEnabled(true);
  }
}

void TileBackgroundRenderer::ClearContextDependentResources(ref_ptr<dp::GraphicsContext> context)
{
  CHECK(context != nullptr, ());

  // Cancel awaiting tile background reading requests for the previous mode.
  for (auto const & tileKey : m_awaitingTiles)
    m_cancelTileBackgroundReadingFn(tileKey, m_currentMode);
  m_awaitingTiles.clear();

  // Release all images (referenced or not) for the previous mode.
  for (auto const & [uid, info] : m_images)
    info.m_texturePool->ReleaseTexture(context, info.m_textureId);
  m_images.clear();
  m_unreferencedLRU.clear();
  m_tiles.clear();
  m_fallbackZoomLevel = TileKey::kNoZoom;
}

void TileBackgroundRenderer::SetBackgroundMode(ref_ptr<dp::GraphicsContext> context, dp::BackgroundMode mode)
{
  if (m_currentMode == mode)
    return;

  m_currentMode = mode;

  if (context == nullptr)
    return;

  ClearContextDependentResources(context);

  if (m_currentMode != dp::BackgroundMode::Default)
    OnUpdateViewport(context, m_lastCoverage, m_lastCurrentZoomLevel);
}

dp::BackgroundMode TileBackgroundRenderer::GetBackgroundMode() const
{
  return m_currentMode;
}

}  // namespace df
