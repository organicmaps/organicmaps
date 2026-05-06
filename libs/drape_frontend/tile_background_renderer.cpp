#include "drape_frontend/tile_background_renderer.hpp"

#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <utility>

namespace df
{
namespace
{
// Max number of refcount==0 images kept alive in the LRU before their texture slots are recycled.
constexpr size_t kMaxUnreferencedImages = 16;
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

  m_state.SetBlending(dp::Blending(false /* isEnabled */));
  m_state.SetDepthTestEnabled(false);

  m_stateArray.SetBlending(dp::Blending(false /* isEnabled */));
  m_stateArray.SetDepthTestEnabled(false);
}

void TileBackgroundRenderer::OnUpdateViewport(ref_ptr<dp::GraphicsContext> context, CoverageResult const & coverage,
                                              int currentZoomLevel, buffer_vector<TileKey, 8> const & tilesToDelete)
{
  m_lastCoverage = coverage;
  m_lastCurrentZoomLevel = currentZoomLevel;
  if (m_currentMode == dp::BackgroundMode::Default)
    return;

  if (context == nullptr)
    return;

  // Cancel awaiting tile background reading requests for deleted tiles, drop their bindings.
  for (auto const & tileKey : tilesToDelete)
  {
    if (m_awaitingTiles.erase(tileKey) > 0)
      m_cancelTileBackgroundReadingFn(tileKey, m_currentMode);

    ReleaseTileBinding(context, tileKey);
  }

  // Request tile background reading for new tiles in the coverage area.
  // The image cache (m_images / unreferenced LRU) is keyed by image uid, which the caller controls,
  // so we always defer the cache check to the read function side: if the image is still alive, the
  // backend's AssignTileBackgroundImage path will dedupe it.
  for (int x = coverage.m_minTileX; x < coverage.m_maxTileX; ++x)
  {
    for (int y = coverage.m_minTileY; y < coverage.m_maxTileY; ++y)
    {
      TileKey const key(x, y, static_cast<uint8_t>(currentZoomLevel));
      if (m_tiles.count(key) == 0 && m_awaitingTiles.insert(key).second)
        m_tileBackgroundReadFn(key, m_currentMode);
    }
  }
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

  // Tile zoom level mismatch — bail out without touching the image.
  if (tileKey.m_zoomLevel != m_lastCurrentZoomLevel)
    return;

  auto imageIt = m_images.find(imageUid);
  if (imageIt == m_images.end())
  {
    LOG(LWARNING, ("SetTileBackgroundData: unknown image uid", imageUid, "for tile", tileKey.Coord2String()));
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

  // Drop any tile bindings for stale zoom levels (mirrors the prior behavior).
  auto it = m_tiles.begin();
  while (it != m_tiles.end())
  {
    if (it->first.m_zoomLevel != tileKey.m_zoomLevel)
    {
      ReleaseImageRef(context, it->second.m_imageUid);
      it = m_tiles.erase(it);
    }
    else
    {
      ++it;
    }
  }
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

void TileBackgroundRenderer::ReleaseTileBinding(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey)
{
  auto it = m_tiles.find(tileKey);
  if (it == m_tiles.end())
    return;

  ReleaseImageRef(context, it->second.m_imageUid);
  m_tiles.erase(it);
}

void TileBackgroundRenderer::Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                    ScreenBase const & screen, int zoomLevel, FrameValues const & frameValues)
{
  if (m_currentMode == dp::BackgroundMode::Default)
    return;

  // Render tiles relative to the screen center.
  frameValues.SetTo(m_programParams);
  auto const pivot = screen.GlobalRect().Center();
  math::Matrix<float, 4, 4> const mv = screen.GetModelView(pivot, 1.0f);
  m_programParams.m_modelView = glsl::make_mat4(mv.m_data);

  struct DrawEntry
  {
    TileKey m_tileKey;
    ImageInfo const * m_image;
    m2::RectF m_rect;
  };

  // Sort tiles by texture pointer to minimize texture switches.
  static std::vector<DrawEntry> sortedTiles;
  sortedTiles.clear();
  sortedTiles.reserve(m_tiles.size());
  for (auto const & [tileKey, binding] : m_tiles)
  {
    if (!screen.ClipRect().IsIntersect(tileKey.GetGlobalRect()))
      continue;
    auto imgIt = m_images.find(binding.m_imageUid);
    if (imgIt == m_images.end())
      continue;
    sortedTiles.push_back({tileKey, &imgIt->second, binding.m_rect});
  }
  if (sortedTiles.empty())
    return;

  std::sort(sortedTiles.begin(), sortedTiles.end(), [](DrawEntry const & lhs, DrawEntry const & rhs)
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
  for (size_t i = 0; i < sortedTiles.size(); ++i)
  {
    auto const & entry = sortedTiles[i];
    auto const r = entry.m_tileKey.GetGlobalRect();
    auto const minR = (m2::PointD(r.minX(), r.minY()) - pivot);
    auto const maxR = (m2::PointD(r.maxX(), r.maxY()) - pivot);
    m_programParams.m_tileCoordsMinMax[instanceIndex] = glsl::vec4(
        static_cast<float>(minR.x), static_cast<float>(minR.y), static_cast<float>(maxR.x), static_cast<float>(maxR.y));
    m_programParams.m_textureRectMinMax[instanceIndex] =
        glsl::vec4(entry.m_rect.minX(), entry.m_rect.minY(), entry.m_rect.maxX(), entry.m_rect.maxY());
    m_programParams.m_textureIndex[instanceIndex] = static_cast<int>(entry.m_image->m_textureId);

    auto const tex = entry.m_image->m_texturePool->GetTexture(entry.m_image->m_textureId);
    bool const nextTextureIsDifferent =
        (i + 1 < sortedTiles.size() &&
         tex != sortedTiles[i + 1].m_image->m_texturePool->GetTexture(sortedTiles[i + 1].m_image->m_textureId));
    if ((instanceIndex + 1) == gpu::kTileBackgroundMaxCount || (i + 1 == sortedTiles.size()) || nextTextureIsDifferent)
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
