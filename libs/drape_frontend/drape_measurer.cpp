#include "drape_frontend/drape_measurer.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include <iomanip>

namespace df
{
DrapeMeasurer & DrapeMeasurer::Instance()
{
  static DrapeMeasurer s_inst;
  return s_inst;
}

void DrapeMeasurer::Start()
{
  using namespace std::chrono;
  if (m_isEnabled)
    return;

  m_isEnabled = true;

  auto currentTime = steady_clock::now();

#ifdef GENERATING_STATISTIC
  m_startScenePreparingTime = currentTime;
  m_maxScenePreparingTime = steady_clock::duration::zero();

  m_startShapesGenTime = currentTime;
  m_totalShapesGenTime = steady_clock::duration::zero();
  m_totalShapesCount = 0;

  m_startOverlayShapesGenTime = currentTime;
  m_totalOverlayShapesGenTime = steady_clock::duration::zero();
  m_totalOverlayShapesCount = 0;
#endif

#ifdef TILES_STATISTIC
  {
    std::lock_guard<std::mutex> lock(m_tilesMutex);
    m_tilesReadInfo.clear();
  }
#endif

#if defined(RENDER_STATISTIC) || defined(TRACK_GPU_MEM)
  m_totalTPF = steady_clock::duration::zero();
  m_totalTPFCount = 0;

  m_minFPS = std::numeric_limits<uint32_t>::max();
  m_fpsDistribution.clear();
  m_totalFPS = 0.0;
  m_totalFPSCount = 0;

  m_startImmediateRenderingTime = currentTime;
  m_immediateRenderingFramesCount = 0;
  m_immediateRenderingTimeSum = steady_clock::duration::zero();
  m_immediateRenderingMinFps = std::numeric_limits<uint32_t>::max();

  m_totalFrameRenderTime = steady_clock::duration::zero();
  m_totalFramesCount = 0;
#endif

  m_startFrameRenderTime = currentTime;

  if (m_realtimeTotalFramesCount == 0)
  {
    m_realtimeMinFrameRenderTime = steady_clock::duration::max();
    m_realtimeMaxFrameRenderTime = steady_clock::duration::min();
    m_realtimeTotalFrameRenderTime = steady_clock::duration::zero();
    m_realtimeSlowFramesCount = 0;
    m_realtimeRenderingBox = {};
  }

#ifdef TRACK_GPU_MEM
  m_maxSnapshotValues = dp::GPUMemTracker::GPUMemorySnapshot();
  m_summarySnapshotValues = dp::GPUMemTracker::GPUMemorySnapshot();
  m_numberOfSnapshots = 0;
#endif
}

void DrapeMeasurer::Stop(bool forceProcessRealtimeStats /* = false */)
{
  using namespace std::chrono;
  if (!m_isEnabled)
    return;

  m_isEnabled = false;

#ifndef DRAPE_MEASURER_BENCHMARK
  if ((forceProcessRealtimeStats && m_realtimeTotalFramesCount > 0) || m_realtimeTotalFramesCount >= 1000)
    m_realtimeTotalFramesCount = 0;
#endif
}

void DrapeMeasurer::SetApiVersion(dp::ApiVersion apiVersion)
{
  m_apiVersion = apiVersion;
}

void DrapeMeasurer::SetResolution(m2::PointU const & resolution)
{
  m_resolution = resolution;
}

void DrapeMeasurer::SetGpuName(std::string const & gpuName)
{
  m_gpuName = gpuName;
}

#ifdef GENERATING_STATISTIC
void DrapeMeasurer::StartScenePreparing()
{
  if (!m_isEnabled)
    return;

  m_startScenePreparingTime = std::chrono::steady_clock::now();
}

void DrapeMeasurer::EndScenePreparing()
{
  if (!m_isEnabled)
    return;

  m_maxScenePreparingTime = max(m_maxScenePreparingTime, std::chrono::steady_clock::now() - m_startScenePreparingTime);
}

void DrapeMeasurer::StartShapesGeneration()
{
  m_startShapesGenTime = std::chrono::steady_clock::now();
}

void DrapeMeasurer::EndShapesGeneration(uint32_t shapesCount)
{
  m_totalShapesGenTime += std::chrono::steady_clock::now() - m_startShapesGenTime;
  m_totalShapesCount += shapesCount;
}

void DrapeMeasurer::StartOverlayShapesGeneration()
{
  m_startOverlayShapesGenTime = std::chrono::steady_clock::now();
}

void DrapeMeasurer::EndOverlayShapesGeneration(uint32_t shapesCount)
{
  m_totalOverlayShapesGenTime += std::chrono::steady_clock::now() - m_startOverlayShapesGenTime;
  m_totalOverlayShapesCount += shapesCount;
}

std::string DrapeMeasurer::GeneratingStatistic::ToString() const
{
  std::ostringstream ss;
  ss << " ----- Generation statistic report ----- \n";
  ss << " Max scene preparing time, ms = " << m_maxScenePreparingTimeInMs << "\n";
  ss << " Shapes total generation time, ms = " << m_shapeGenTimeInMs << "\n";
  ss << " Shapes total count = " << m_shapesCount << ", (" << static_cast<double>(m_shapeGenTimeInMs) / m_shapesCount
     << " ms per shape)\n";
  ss << " Overlay shapes total generation time, ms = " << m_overlayShapeGenTimeInMs << "\n";
  ss << " Overlay shapes total count = " << m_overlayShapesCount << ", ("
     << static_cast<double>(m_overlayShapeGenTimeInMs) / m_overlayShapesCount << " ms per overlay)\n";
  ss << " ----- Generation statistic report ----- \n";

  return ss.str();
}

DrapeMeasurer::GeneratingStatistic DrapeMeasurer::GetGeneratingStatistic()
{
  using namespace std::chrono;

  GeneratingStatistic statistic;
  statistic.m_shapesCount = m_totalShapesCount;
  statistic.m_shapeGenTimeInMs = static_cast<uint32_t>(duration_cast<milliseconds>(m_totalShapesGenTime).count());

  statistic.m_overlayShapesCount = m_totalOverlayShapesCount;
  statistic.m_overlayShapeGenTimeInMs =
      static_cast<uint32_t>(duration_cast<milliseconds>(m_totalOverlayShapesGenTime).count());

  statistic.m_maxScenePreparingTimeInMs =
      static_cast<uint32_t>(duration_cast<milliseconds>(m_maxScenePreparingTime).count());

  return statistic;
}
#endif

#ifdef RENDER_STATISTIC
std::string DrapeMeasurer::RenderStatistic::ToString() const
{
  std::ostringstream ss;
  ss << " ----- Render statistic report ----- \n";
  ss << " FPS = " << m_FPS << "\n";
  ss << " Min FPS = " << m_minFPS << "\n";
  ss << " Immediate rendering FPS = " << m_immediateRenderingFPS << "\n";
  ss << " Immediate rendering min FPS = " << m_immediateRenderingMinFPS << "\n";
  ss << " Frame render time, ms = " << m_frameRenderTimeInMs << "\n";
  if (!m_fpsDistribution.empty())
  {
    ss << " FPS Distribution:\n";
    for (auto const & fps : m_fpsDistribution)
    {
      ss << "   " << fps.first << "-" << (fps.first + 10) << ": " << std::setprecision(4) << fps.second * 100.0f
         << "%\n";
    }
  }
  ss << " ----- Render statistic report ----- \n";

  return ss.str();
}

DrapeMeasurer::RenderStatistic DrapeMeasurer::GetRenderStatistic()
{
  using namespace std::chrono;

  RenderStatistic statistic;
  statistic.m_FPS = static_cast<uint32_t>(m_totalFPS / m_totalFPSCount);
  statistic.m_minFPS = m_minFPS;
  statistic.m_frameRenderTimeInMs =
      static_cast<uint32_t>(duration_cast<milliseconds>(m_totalTPF).count()) / m_totalTPFCount;

  uint32_t totalCount = 0;
  for (auto const & fps : m_fpsDistribution)
    totalCount += fps.second;

  if (totalCount != 0)
    for (auto const & fps : m_fpsDistribution)
      statistic.m_fpsDistribution[fps.first] = static_cast<float>(fps.second) / totalCount;

  statistic.m_immediateRenderingMinFPS = m_immediateRenderingMinFps;
  if (m_immediateRenderingFramesCount > 0)
  {
    auto const timeSumMs = duration_cast<milliseconds>(m_immediateRenderingTimeSum).count();
    auto const avgFrameTimeMs = static_cast<double>(timeSumMs) / m_immediateRenderingFramesCount;
    statistic.m_immediateRenderingFPS = static_cast<uint32_t>(1000.0 / avgFrameTimeMs);
  }

  return statistic;
}
#endif

void DrapeMeasurer::BeforeRenderFrame()
{
  if (!m_isEnabled)
    return;

  m_startFrameRenderTime = std::chrono::steady_clock::now();
}

void DrapeMeasurer::AfterRenderFrame(bool isActiveFrame, m2::PointD const & viewportCenter)
{
  using namespace std::chrono;

  if (!m_isEnabled)
    return;

  auto const frameTime = steady_clock::now() - m_startFrameRenderTime;
  if (isActiveFrame)
  {
    if (mercator::Bounds::FullRect().IsPointInside(viewportCenter))
      m_realtimeRenderingBox.Add(viewportCenter);
    m_realtimeTotalFrameRenderTime += frameTime;
    m_realtimeMinFrameRenderTime = std::min(m_realtimeMinFrameRenderTime, frameTime);
    m_realtimeMaxFrameRenderTime = std::max(m_realtimeMaxFrameRenderTime, frameTime);

    auto const frameTimeMs = duration_cast<milliseconds>(frameTime).count();
    if (frameTimeMs > 30)
      ++m_realtimeSlowFramesCount;

    ++m_realtimeTotalFramesCount;
  }

#if defined(RENDER_STATISTIC) || defined(TRACK_GPU_MEM)
  ++m_totalFramesCount;
  m_totalFrameRenderTime += frameTime;

  auto const elapsed = duration_cast<milliseconds>(m_totalFrameRenderTime).count();
  if (elapsed >= 30)
  {
    double fps = m_totalFramesCount * 1000.0 / elapsed;
    m_minFPS = std::min(m_minFPS, static_cast<uint32_t>(fps));

    m_totalFPS += fps;
    ++m_totalFPSCount;

    m_totalTPF += m_totalFrameRenderTime / m_totalFramesCount;
    ++m_totalTPFCount;

    auto const fpsGroup = (static_cast<uint32_t>(fps) / 10) * 10;
    m_fpsDistribution[fpsGroup]++;

    m_totalFramesCount = 0;
    m_totalFrameRenderTime = steady_clock::duration::zero();

#ifdef TRACK_GPU_MEM
    TakeGPUMemorySnapshot();
#endif
  }
#endif
}

#if defined(RENDER_STATISTIC) || defined(TRACK_GPU_MEM)
void DrapeMeasurer::BeforeImmediateRendering()
{
  if (!m_isEnabled)
    return;

  m_startImmediateRenderingTime = std::chrono::steady_clock::now();
}

void DrapeMeasurer::AfterImmediateRendering()
{
  using namespace std::chrono;

  if (!m_isEnabled)
    return;

  ++m_immediateRenderingFramesCount;

  auto const elapsed = steady_clock::now() - m_startImmediateRenderingTime;
  m_immediateRenderingTimeSum += elapsed;
  auto const elapsedMs = duration_cast<milliseconds>(elapsed).count();
  if (elapsedMs > 0)
  {
    auto const fps = static_cast<uint32_t>(1000 / elapsedMs);
    m_immediateRenderingMinFps = std::min(m_immediateRenderingMinFps, fps);
  }
}
#endif

#ifdef TILES_STATISTIC
std::string DrapeMeasurer::TileStatistic::ToString() const
{
  std::ostringstream ss;
  ss << " ----- Tiles read statistic report ----- \n";
  ss << " Tile read time, ms = " << m_tileReadTimeInMs << "\n";
  ss << " Tiles count = " << m_totalTilesCount << "\n";
  ss << " ----- Tiles read statistic report ----- \n";

  return ss.str();
}

void DrapeMeasurer::StartTileReading()
{
  if (!m_isEnabled)
    return;

  threads::ThreadID tid = threads::GetCurrentThreadID();
  std::shared_ptr<TileReadInfo> tileInfo;
  {
    std::lock_guard<std::mutex> lock(m_tilesMutex);
    auto const it = m_tilesReadInfo.find(tid);
    if (it != m_tilesReadInfo.end())
    {
      tileInfo = it->second;
    }
    else
    {
      tileInfo = std::make_shared<TileReadInfo>();
      m_tilesReadInfo.insert(make_pair(tid, tileInfo));
    }
  }

  auto const currentTime = std::chrono::steady_clock::now();
  tileInfo->m_startTileReadTime = currentTime;
}

void DrapeMeasurer::EndTileReading()
{
  if (!m_isEnabled)
    return;

  auto const currentTime = std::chrono::steady_clock::now();

  threads::ThreadID tid = threads::GetCurrentThreadID();
  std::shared_ptr<TileReadInfo> tileInfo;
  {
    std::lock_guard<std::mutex> lock(m_tilesMutex);
    auto const it = m_tilesReadInfo.find(tid);
    if (it != m_tilesReadInfo.end())
      tileInfo = it->second;
    else
      return;
  }

  auto passedTime = currentTime - tileInfo->m_startTileReadTime;
  tileInfo->m_totalTileReadTime += passedTime;
  ++tileInfo->m_totalTilesCount;
}

DrapeMeasurer::TileStatistic DrapeMeasurer::GetTileStatistic()
{
  using namespace std::chrono;
  TileStatistic statistic;
  {
    std::lock_guard<std::mutex> lock(m_tilesMutex);
    for (auto const & it : m_tilesReadInfo)
    {
      statistic.m_tileReadTimeInMs +=
          static_cast<uint32_t>(duration_cast<milliseconds>(it.second->m_totalTileReadTime).count());
      statistic.m_totalTilesCount += it.second->m_totalTilesCount;
    }
  }
  if (statistic.m_totalTilesCount > 0)
    statistic.m_tileReadTimeInMs /= statistic.m_totalTilesCount;

  return statistic;
}
#endif

#ifdef TRACK_GPU_MEM
std::string DrapeMeasurer::GPUMemoryStatistic::ToString() const
{
  std::ostringstream ss;
  ss << " ----- GPU memory report ----- \n";
  ss << " --Max memory values:\n";
  ss << m_maxMemoryValues.ToString();
  ss << "\n --Average memory values:\n";
  ss << m_averageMemoryValues.ToString();
  ss << " ----- GPU memory report ----- \n";

  return ss.str();
}

void DrapeMeasurer::TakeGPUMemorySnapshot()
{
  dp::GPUMemTracker::GPUMemorySnapshot snap = dp::GPUMemTracker::Inst().GetMemorySnapshot();
  for (auto const & tagPair : snap.m_tagStats)
  {
    auto itMax = m_maxSnapshotValues.m_tagStats.find(tagPair.first);
    if (itMax != m_maxSnapshotValues.m_tagStats.end())
    {
      itMax->second.m_objectsCount = std::max(itMax->second.m_objectsCount, tagPair.second.m_objectsCount);
      itMax->second.m_alocatedInMb = std::max(itMax->second.m_alocatedInMb, tagPair.second.m_alocatedInMb);
      itMax->second.m_usedInMb = std::max(itMax->second.m_alocatedInMb, tagPair.second.m_usedInMb);
    }
    else
    {
      m_maxSnapshotValues.m_tagStats.insert(tagPair);
    }

    auto itSummary = m_summarySnapshotValues.m_tagStats.find(tagPair.first);
    if (itSummary != m_summarySnapshotValues.m_tagStats.end())
    {
      itSummary->second.m_objectsCount += tagPair.second.m_objectsCount;
      itSummary->second.m_alocatedInMb += tagPair.second.m_alocatedInMb;
      itSummary->second.m_usedInMb += tagPair.second.m_usedInMb;
    }
    else
    {
      m_summarySnapshotValues.m_tagStats.insert(tagPair);
    }
  }

  m_maxSnapshotValues.m_summaryAllocatedInMb =
      std::max(snap.m_summaryAllocatedInMb, m_maxSnapshotValues.m_summaryAllocatedInMb);
  m_maxSnapshotValues.m_summaryUsedInMb = std::max(snap.m_summaryUsedInMb, m_maxSnapshotValues.m_summaryUsedInMb);

  m_summarySnapshotValues.m_summaryAllocatedInMb += snap.m_summaryAllocatedInMb;
  m_summarySnapshotValues.m_summaryUsedInMb += snap.m_summaryUsedInMb;

  ++m_numberOfSnapshots;
}

DrapeMeasurer::GPUMemoryStatistic DrapeMeasurer::GetGPUMemoryStatistic()
{
  GPUMemoryStatistic statistic;
  statistic.m_maxMemoryValues = m_maxSnapshotValues;

  statistic.m_averageMemoryValues = m_summarySnapshotValues;
  if (m_numberOfSnapshots > 0)
  {
    for (auto & tagPair : statistic.m_averageMemoryValues.m_tagStats)
    {
      tagPair.second.m_objectsCount /= m_numberOfSnapshots;
      tagPair.second.m_alocatedInMb /= m_numberOfSnapshots;
      tagPair.second.m_usedInMb /= m_numberOfSnapshots;
    }
    statistic.m_averageMemoryValues.m_summaryAllocatedInMb /= m_numberOfSnapshots;
    statistic.m_averageMemoryValues.m_summaryUsedInMb /= m_numberOfSnapshots;
  }
  return statistic;
}
#endif

std::string DrapeMeasurer::DrapeStatistic::ToString() const
{
  std::ostringstream ss;
  ss << "\n ===== Drape statistic report ===== \n";
#ifdef RENDER_STATISTIC
  ss << "\n" << m_renderStatistic.ToString() << "\n";
#endif
#ifdef TILES_STATISTIC
  ss << "\n" << m_tileStatistic.ToString() << "\n";
#endif
#ifdef GENERATING_STATISTIC
  ss << "\n" << m_generatingStatistic.ToString() << "\n";
#endif
#ifdef TRACK_GPU_MEM
  ss << "\n" << m_gpuMemStatistic.ToString() << "\n";
#endif
#ifdef TRACK_GLYPH_USAGE
  ss << "\n" << m_glyphStatistic.ToString() << "\n";
#endif
  ss << "\n ===== Drape statistic report ===== \n\n";

  return ss.str();
}

DrapeMeasurer::DrapeStatistic DrapeMeasurer::GetDrapeStatistic()
{
  DrapeStatistic statistic;
#ifdef RENDER_STATISTIC
  statistic.m_renderStatistic = GetRenderStatistic();
#endif
#ifdef TILES_STATISTIC
  statistic.m_tileStatistic = GetTileStatistic();
#endif
#ifdef GENERATING_STATISTIC
  statistic.m_generatingStatistic = GetGeneratingStatistic();
#endif
#ifdef TRACK_GPU_MEM
  statistic.m_gpuMemStatistic = GetGPUMemoryStatistic();
#endif
#ifdef TRACK_GLYPH_USAGE
  statistic.m_glyphStatistic = dp::GlyphUsageTracker::Instance().Report();
#endif
  return statistic;
}
}  // namespace df
