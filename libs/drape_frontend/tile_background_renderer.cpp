#include "drape_frontend/tile_background_renderer.hpp"

#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "base/logging.hpp"

#include <algorithm>

namespace df
{
TileBackgroundRenderer::TileBackgroundRenderer(
    MapDataProvider::TTileBackgroundReadFn && tileBackgroundReadFn,
    MapDataProvider::TCancelTileBackgroundReadingFn && cancelTileBackgroundReadingFn)
  : m_tileBackgroundReadFn(std::move(tileBackgroundReadFn))
  , m_cancelTileBackgroundReadingFn(std::move(cancelTileBackgroundReadingFn))
  , m_state(CreateRenderState(gpu::Program::TileBackground, DepthLayer::GeometryLayer))
  , m_instancing(std::make_unique<dp::Instancing>())
{
  CHECK(m_tileBackgroundReadFn != nullptr, ());
  CHECK(m_cancelTileBackgroundReadingFn != nullptr, ());

  m_state.SetBlending(dp::Blending(false /* isEnabled */));
  m_state.SetDepthTestEnabled(false);
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

  // Ignore textures for wrong background mode and zoom level
  if (mode != m_currentMode || tileKey.m_zoomLevel != m_lastCurrentZoomLevel)
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

  // Remove tiles with different zoom levels
  auto tileIt = m_tileTextures.begin();
  while (tileIt != m_tileTextures.end())
  {
    if (tileIt->first.m_zoomLevel != tileKey.m_zoomLevel)
    {
      tileIt->second.m_texturePool->ReleaseTexture(context, tileIt->second.m_textureId);
      tileIt = m_tileTextures.erase(tileIt);
    }
    else
    {
      ++tileIt;
    }
  }

  m_tileTextures[tileKey] = {texturePool, textureId};
  m_awaitingTiles.erase(it);
}

void TileBackgroundRenderer::Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                    ScreenBase const & screen, int zoomLevel, FrameValues const & frameValues)
{
  if (m_currentMode == dp::BackgroundMode::Default)
    return;

  auto program = mng->GetProgram(m_state.GetProgram<gpu::Program>());
  bool programBound = false;

  // Render tiles relative to the screen center.
  frameValues.SetTo(m_programParams);
  auto const pivot = screen.GlobalRect().Center();
  math::Matrix<float, 4, 4> const mv = screen.GetModelView(pivot, 1.0f);
  m_programParams.m_modelView = glsl::make_mat4(mv.m_data);

  // Sort tiles by texture pointer to minimize texture switches.
  static std::vector<std::pair<TileKey, TextureInfo>> sortedTiles;
  sortedTiles.assign(m_tileTextures.begin(), m_tileTextures.end());
  std::sort(sortedTiles.begin(), sortedTiles.end(), [](auto const & lhs, auto const & rhs)
  {
    auto const lhsTex = lhs.second.m_texturePool->GetTexture(lhs.second.m_textureId);
    auto const rhsTex = rhs.second.m_texturePool->GetTexture(rhs.second.m_textureId);
    if (lhsTex == rhsTex)
      return lhs.first < rhs.first;
    return lhsTex < rhsTex;
  });

  uint32_t instanceIndex = 0;
  ref_ptr<dp::Texture> prevTex = nullptr;
  for (size_t i = 0; i < sortedTiles.size(); ++i)
  {
    auto const & [tileKey, textureInfo] = sortedTiles[i];
    auto const r = tileKey.GetGlobalRect();
    if (!screen.ClipRect().IsIntersect(r))
      continue;

    auto const minR = (m2::PointD(r.minX(), r.minY()) - pivot);
    auto const maxR = (m2::PointD(r.maxX(), r.maxY()) - pivot);
    m_programParams.m_tileCoordsMinMax[instanceIndex] = glsl::vec4(
        static_cast<float>(minR.x), static_cast<float>(minR.y), static_cast<float>(maxR.x), static_cast<float>(maxR.y));
    m_programParams.m_textureIndex[instanceIndex] = static_cast<int>(textureInfo.m_textureId);

    auto const tex = textureInfo.m_texturePool->GetTexture(textureInfo.m_textureId);
    bool const nextTextureIsDifferent =
        (i + 1 < sortedTiles.size() &&
         tex != sortedTiles[i + 1].second.m_texturePool->GetTexture(sortedTiles[i + 1].second.m_textureId));
    if ((instanceIndex + 1) == gpu::kTileBackgroundMaxCount || (i + 1 == sortedTiles.size()) || nextTextureIsDifferent)
    {
      m_state.SetColorTexture(tex);
      if (!programBound)
      {
        context->SetCullingEnabled(false);
        program->Bind();
        programBound = true;
      }
      dp::ApplyState(context, program, m_state);
      mng->GetParamsSetter()->Apply(context, program, m_programParams);

      m_instancing->DrawInstancedTriangleStrip(context, instanceIndex + 1, 4);

      // Restart filling from the beginning
      instanceIndex = 0;
    }
    else
    {
      ++instanceIndex;
    }
    prevTex = tex;
  }

  if (programBound)
  {
    program->Unbind();
    context->SetCullingEnabled(true);
  }
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
