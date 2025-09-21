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

void TileBackgroundRenderer::OnUpdateViewport(ref_ptr<dp::GraphicsContext> context, CoverageResult const & coverage,
                                              int currentZoomLevel, buffer_vector<TileKey, 8> const & tilesToDelete)
{
  m_lastCoverage = coverage;
  m_lastCurrentZoomLevel = currentZoomLevel;
  if (m_currentMode == dp::BackgroundMode::Default)
    return;

  // Cancel awaiting tile background reading requests for deleted tiles
  for (auto const & tileKey : tilesToDelete)
  {
    if (m_awaitingTiles.erase(tileKey) > 0)
      m_cancelTileBackgroundReadingFn(tileKey, m_currentMode);

    // Remove textures for deleted tiles
    // TODO: Move texture data to LRU cache instead of deleting it
    auto it = m_tileTextures.find(tileKey);
    if (it != m_tileTextures.end())
    {
      if (context != nullptr)
        it->second.m_texturePool->ReleaseTexture(context, it->second.m_textureId);
      m_tileTextures.erase(it);
    }
  }

  // Request tile background reading for new tiles in the coverage area
  for (int x = coverage.m_minTileX; x < coverage.m_maxTileX; ++x)
  {
    for (int y = coverage.m_minTileY; y < coverage.m_maxTileY; ++y)
    {
      TileKey const key(x, y, static_cast<uint8_t>(currentZoomLevel));
      if (m_tileTextures.count(key) == 0 && m_awaitingTiles.insert(key).second)
        m_tileBackgroundReadFn(key, m_currentMode);
    }
  }
}

void TileBackgroundRenderer::AssignTileBackgroundTexture(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                                                         ref_ptr<dp::TexturePool> texturePool,
                                                         dp::TexturePool::TextureId textureId, dp::BackgroundMode mode)
{
  if (context == nullptr)
    return;

  // Ignore textures for wrong background mode
  if (mode != m_currentMode)
  {
    texturePool->ReleaseTexture(context, textureId);
    return;
  }

  // Ignore textures for tiles that are not awaited
  auto it = m_awaitingTiles.find(tileKey);
  if (it == m_awaitingTiles.end())
  {
    texturePool->ReleaseTexture(context, textureId);
    return;
  }

  // Texture should not be re-assigned. Performance may degrade because of it.
  // If you see the warning message, you probably call DrapeEngine::SetTileBackgroundData() not correctly.
  // You should call it only once for each tileKey when the tile background image data reading is finished.
  if (auto it = m_tileTextures.find(tileKey); it != m_tileTextures.end())
  {
    LOG(LWARNING, ("Tile background texture for tile ", tileKey.Coord2String(), " is already assigned"));
    auto & info = it->second;
    info.m_texturePool->ReleaseTexture(context, info.m_textureId);
  }

  m_tileTextures[tileKey] = {texturePool, textureId};
  m_awaitingTiles.erase(it);
}

void TileBackgroundRenderer::Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                    ScreenBase const & screen, int zoomLevel, FrameValues const & frameValues)
{
  if (m_currentMode == dp::BackgroundMode::Default)
    return;

  // TODO
}

void TileBackgroundRenderer::ClearContextDependentResources(ref_ptr<dp::GraphicsContext> context)
{
  CHECK(context != nullptr, ());

  // Cancel awaiting tile background reading requests for the previous mode
  for (auto const & tileKey : m_awaitingTiles)
    m_cancelTileBackgroundReadingFn(tileKey, m_currentMode);
  m_awaitingTiles.clear();

  // Release all tile background textures for the previous mode
  for (auto const & [tileKey, info] : m_tileTextures)
    info.m_texturePool->ReleaseTexture(context, info.m_textureId);
  m_tileTextures.clear();
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
    OnUpdateViewport(context, m_lastCoverage, m_lastCurrentZoomLevel, {});
}

dp::BackgroundMode TileBackgroundRenderer::GetBackgroundMode() const
{
  return m_currentMode;
}
}  // namespace df
