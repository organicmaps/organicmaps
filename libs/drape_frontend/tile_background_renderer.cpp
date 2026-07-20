#include "drape_frontend/tile_background_renderer.hpp"

#include "drape_frontend/render_state_extension.hpp"

#include "base/assert.hpp"

#include <algorithm>

namespace df
{
namespace
{
// Max number of refcount==0 images kept alive in the LRU before their texture slots are recycled.
constexpr size_t kMaxUnreferencedImages = 16;

// Background coordinates use the true zoom, so their rects must not be clipped to the vector data zoom.
constexpr bool kClipByDataMaxZoom = false;

// Background bindings are coordinate-keyed; visibility comes from this coverage, not vector tilesToDelete.
bool IsInCoverage(TileKey const & tileKey, CoverageResult const & coverage, uint8_t zoomLevel)
{
  return tileKey.m_zoomLevel == zoomLevel && tileKey.m_x >= coverage.m_minTileX && tileKey.m_x < coverage.m_maxTileX &&
         tileKey.m_y >= coverage.m_minTileY && tileKey.m_y < coverage.m_maxTileY;
}

// Compare retained tiles from other zoom levels against the coverage geometrically.
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

  // Transparent raster pixels must let the map below show through.
  m_state.SetBlending(dp::Blending(true /* isEnabled */));
  m_state.SetDepthTestEnabled(false);

  m_stateArray.SetBlending(dp::Blending(true /* isEnabled */));
  m_stateArray.SetDepthTestEnabled(false);
}

void TileBackgroundRenderer::OnUpdateViewport(ref_ptr<dp::GraphicsContext> context, CoverageResult coverage,
                                              uint8_t currentZoomLevel)
{
  // Viewport inflation can extend beyond the poles; discard rows outside the source's world extent.
  int const minY = currentZoomLevel > 1 ? -(1 << (currentZoomLevel - 2)) : -1;
  int const maxY = currentZoomLevel > 1 ? (1 << (currentZoomLevel - 2)) : 1;
  coverage.m_minTileY = std::max(coverage.m_minTileY, minY);
  coverage.m_maxTileY = std::min(coverage.m_maxTileY, maxY);

  uint8_t const prevZoomLevel = m_lastCurrentZoomLevel;
  m_lastCoverage = coverage;
  m_lastCurrentZoomLevel = currentZoomLevel;
  if (m_currentMode == dp::BackgroundMode::Default)
    return;

  if (context == nullptr)
    return;

  // Keep the best-covered previous level while the new one loads. A partly loaded intermediate
  // level must not displace a better fallback during a fast pinch.
  if (currentZoomLevel != prevZoomLevel)
  {
    auto const viewportRect = CalcCoverageRect(coverage, currentZoomLevel);
    if (CoveredFraction(prevZoomLevel, viewportRect) >= CoveredFraction(m_fallbackZoomLevel, viewportRect))
      m_fallbackZoomLevel = prevZoomLevel;
    if (m_fallbackZoomLevel == currentZoomLevel)
      m_fallbackZoomLevel = TileKey::kNoZoom;  // zoomed back onto it: it is the current level again
  }

  // Forget requests outside the current coverage, releasing any pending upload references.
  for (auto it = m_tileRequests.begin(); it != m_tileRequests.end();)
  {
    if (IsInCoverage(it->first, coverage, currentZoomLevel))
    {
      ++it;
      continue;
    }

    TileKey const tileKey(it->first, it->second.m_generation, 0 /* userMarksGeneration */);
    if (!it->second.m_imageUid.empty())
      ReleaseImageRef(context, it->second.m_imageUid);
    it = m_tileRequests.erase(it);
    m_cancelTileBackgroundReadingFn(tileKey, m_currentMode);
  }

  // Keep current bindings and the overlapping fallback; release everything else.
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

  // Track accepted reads, retaining failed cells until they leave coverage or the layer resets.
  for (int x = coverage.m_minTileX; x < coverage.m_maxTileX; ++x)
  {
    for (int y = coverage.m_minTileY; y < coverage.m_maxTileY; ++y)
    {
      TileKey const key(x, y, currentZoomLevel);
      if (m_tiles.count(key) != 0 || m_tileRequests.count(key) != 0)
        continue;

      TileKey const requestKey(key, ++m_nextRequestGeneration, 0 /* userMarksGeneration */);
      if (m_tileBackgroundReadFn(requestKey, m_currentMode))
        m_tileRequests.emplace(key, TileRequest{requestKey.m_generation, {}});
    }
  }

  RetireFallbackIfReady(context);
}

double TileBackgroundRenderer::CoveredFraction(uint8_t zoomLevel, m2::RectD const & viewportRect) const
{
  double const viewportArea = viewportRect.SizeX() * viewportRect.SizeY();
  if (zoomLevel == TileKey::kNoZoom || viewportArea <= 0.0)
    return 0.0;

  // Bound tiles do not overlap. Sum their viewport intersections rather than enumerating a grid
  // that grows exponentially with the zoom difference.
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

void TileBackgroundRenderer::GetTileRects(TileKey const & tileKey, m2::RectF const & textureRect,
                                          std::vector<TileRect> & rects) const
{
  int const zoomDiff = static_cast<int>(m_lastCurrentZoomLevel) - static_cast<int>(tileKey.m_zoomLevel);
  if (zoomDiff <= 0)
  {
    if (zoomDiff < 0 &&
        m_tiles.count(TileKey(tileKey.m_x >> -zoomDiff, tileKey.m_y >> -zoomDiff, m_lastCurrentZoomLevel)) != 0)
      return;

    rects.push_back({tileKey, textureRect});
    return;
  }

  // Split a coarser fallback into missing current-level cells. Iterating only the coverage keeps
  // large zoom jumps bounded by a screenful, rather than all descendants of the fallback tile.
  int const cells = 1 << zoomDiff;
  int const minX = std::max(tileKey.m_x * cells, m_lastCoverage.m_minTileX);
  int const maxX = std::min((tileKey.m_x + 1) * cells, m_lastCoverage.m_maxTileX);
  int const minY = std::max(tileKey.m_y * cells, m_lastCoverage.m_minTileY);
  int const maxY = std::min((tileKey.m_y + 1) * cells, m_lastCoverage.m_maxTileY);
  auto const parentRect = tileKey.GetGlobalRect(kClipByDataMaxZoom);
  for (int x = minX; x < maxX; ++x)
  {
    for (int y = minY; y < maxY; ++y)
    {
      TileKey const key(x, y, m_lastCurrentZoomLevel);
      if (m_tiles.count(key) != 0)
        continue;

      auto const r = key.GetGlobalRect(kClipByDataMaxZoom);
      auto const u = [&](double x)
      {
        return static_cast<float>(textureRect.minX() +
                                  (x - parentRect.minX()) / parentRect.SizeX() * textureRect.SizeX());
      };
      auto const v = [&](double y)
      {
        return static_cast<float>(textureRect.minY() +
                                  (y - parentRect.minY()) / parentRect.SizeY() * textureRect.SizeY());
      };
      rects.push_back({key, {u(r.minX()), v(r.minY()), u(r.maxX()), v(r.maxY())}});
    }
  }
}

void TileBackgroundRenderer::RetireFallbackIfReady(ref_ptr<dp::GraphicsContext> context)
{
  // The viewport sweep keeps only current-level requests; failures still need the fallback.
  if (m_fallbackZoomLevel == TileKey::kNoZoom || !m_tileRequests.empty())
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

void TileBackgroundRenderer::AssignTileBackgroundImage(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                                                       std::string const & uid, ref_ptr<dp::TexturePool> texturePool,
                                                       dp::TexturePool::TextureId textureId, dp::BackgroundMode mode)
{
  ASSERT(!uid.empty(), ());
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
  }
  else
  {
    m_images.emplace(uid, ImageInfo{texturePool, textureId, 0});
    m_unreferencedLRU.push_back(uid);
  }

  TileKey const coordKey(tileKey.m_x, tileKey.m_y, tileKey.m_zoomLevel);
  auto const requestIt = m_tileRequests.find(coordKey);
  if (requestIt != m_tileRequests.end() && requestIt->second.m_generation == tileKey.m_generation)
  {
    ASSERT(requestIt->second.m_imageUid.empty(), (tileKey));
    requestIt->second.m_imageUid = uid;
    AcquireImageRef(uid);
  }

  TrimUnreferencedImages(context);
}

void TileBackgroundRenderer::SetTileBackgroundData(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                                                   std::string const & imageUid, m2::RectF const & rect)
{
  if (context == nullptr)
    return;

  // TileKey hashing includes generations, so use a coordinate-only key and check generation separately.
  TileKey const coordKey(tileKey.m_x, tileKey.m_y, tileKey.m_zoomLevel);
  auto const requestIt = m_tileRequests.find(coordKey);
  if (requestIt == m_tileRequests.end() || requestIt->second.m_generation != tileKey.m_generation)
    return;  // Completion of a cancelled/replaced request.
  ASSERT_EQUAL(requestIt->second.m_imageUid, imageUid, (tileKey));
  if (imageUid.empty())
    return;
  m_tileRequests.erase(requestIt);

  ASSERT(IsInCoverage(tileKey, m_lastCoverage, m_lastCurrentZoomLevel), (tileKey));
  ASSERT(m_images.contains(imageUid), (imageUid));
  // The binding takes over the reference acquired on upload.
  VERIFY(m_tiles.emplace(coordKey, TileBinding{imageUid, rect}).second, (tileKey));

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
    TrimUnreferencedImages(context);
  }
}

void TileBackgroundRenderer::TrimUnreferencedImages(ref_ptr<dp::GraphicsContext> context)
{
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
    auto imgIt = m_images.find(binding.m_imageUid);
    if (imgIt == m_images.end())
      continue;

    m_tileRects.clear();
    GetTileRects(tileKey, binding.m_rect, m_tileRects);
    for (auto const & rect : m_tileRects)
      m_sortedTiles.push_back({rect.m_tileKey, &imgIt->second, rect.m_textureRect});
  }
  if (m_sortedTiles.empty())
    return;

  std::sort(m_sortedTiles.begin(), m_sortedTiles.end(), [](DrawEntry const & lhs, DrawEntry const & rhs)
  {
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

  // Clear request history and cancel unresolved reads.
  for (auto const & [tileKey, request] : m_tileRequests)
    m_cancelTileBackgroundReadingFn(TileKey(tileKey, request.m_generation, 0 /* userMarksGeneration */), m_currentMode);
  m_tileRequests.clear();

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
