#pragma once

#include "render_policy.hpp"
#include "tiler.hpp"
#include "tile.hpp"
#include "window_handle.hpp"

#include "geometry/screenbase.hpp"

#include "graphics/overlay.hpp"

#include "base/thread.hpp"
#include "base/threaded_list.hpp"
#include "base/mutex.hpp"
#include "base/commands_queue.hpp"

#include "std/vector.hpp"
#include "std/shared_ptr.hpp"

class TileRenderer;
class TileCache;
class ScreenCoverage;

namespace graphics
{
  class Skin;

  namespace gl
  {
    class Screen;
  }
  class ResourceStyleCache;
}

/// Control class for TilingRenderPolicy. It processes request from the RenderPolicy
/// to draw a specific ScreenBase by splitting it in tiles that is not rendered now and
/// feeding them into TileRenderer. Each command to render a tile is enqueued with
/// a small command, which is feeding the CoverageGenerator with the command to process
/// newly rendered tile(p.e. merge it into current ScreenCoverage).
class CoverageGenerator
{
public:

  CoverageGenerator(TileRenderer * tileRenderer,
                    shared_ptr<WindowHandle> const & windowHandle,
                    shared_ptr<graphics::RenderContext> const & primaryRC,
                    shared_ptr<graphics::ResourceManager> const & rm,
                    graphics::PacketsQueue * glQueue);

  ~CoverageGenerator();

  void Shutdown();

  //@{ Add task to run on CoverageGenerator thread
  void InvalidateTiles(m2::AnyRectD const & rect, int startScale);
  void CoverScreen(ScreenBase const & screen, bool doForce);
  //}@

  //@{ Benchmark support
  int InsertBenchmarkFence();
  void JoinBenchmarkFence(int fenceID);
  //}@

  void Draw(graphics::Screen * s, ScreenBase const & screen);

  //@{ Frame lock
  void Lock();
  void Unlock();
  //}@

  void Pause();
  void Resume();

  graphics::Overlay * GetOverlay() const;

  bool IsEmptyDrawing() const;
  bool DoForceUpdate() const;

private:
  //@{ Called only on android, with Single thread policy
  void InitializeThreadGL(shared_ptr<graphics::RenderContext> context,
                          shared_ptr<graphics::ResourceManager> resourceManager,
                          graphics::PacketsQueue * glQueue);
  void FinalizeThreadGL(shared_ptr<graphics::RenderContext> context,
                        shared_ptr<graphics::ResourceManager> resourceManager);
  //@}

  void MergeTile(Tiler::RectInfo const & rectInfo, int sequenceID);

private:
  void CoverScreenImpl(core::CommandsQueue::Environment const & env,
                       ScreenBase const & screen,
                       int sequenceID);

  void MergeTileImpl(core::CommandsQueue::Environment const & env,
                     Tiler::RectInfo const & rectInfo,
                     int sequenceID);

  void InvalidateTilesImpl(m2::AnyRectD const & rect, int startScale);

  bool IsBackEmptyDrawing() const;

private:
  void FinishSequenceIfNeeded();
  void ComputeCoverTasks();
  void MergeOverlay();
  void MergeSingleTile(Tiler::RectInfo const & rectInfo);
  bool CacheCoverage(core::CommandsQueue::Environment const & env);

  void ClearCoverage();

private:
  struct BenchmarkInfo
  {
    int m_benchmarkSequenceID;
    int m_tilesCount;

    FenceManager m_fenceManager;
    int m_currentFenceID;

    bool m_isBenchmarking;

    BenchmarkInfo();

    void DecrementTileCount(int sequenceID);

    int InsertBenchmarkFence();
    void JoinBenchmarkFence(int fenceID);
    void SignalBenchmarkFence();
    void TryFinishSequence();
  } m_benchmarkInfo;

  struct StateInfo
  {
    bool m_isPause;
    bool m_needForceUpdate;
    int m_sequenceID;
    ScreenBase m_currentScreen;
    threads::Mutex m_mutex;

    StateInfo();

    void SetSequenceID(int sequenceID);
    void SetForceUpdate(bool needForceUpdate);

    void Pause();
    void Resume();
  } m_stateInfo;

  struct CoverageInfo
  {
    CoverageInfo(TileRenderer * tileRenderer);
    ~CoverageInfo();

    Tiler m_tiler;
    TileRenderer * m_tileRenderer;

    typedef set<Tile const *, LessRectInfo> TTileSet;
    TTileSet m_tiles;

    graphics::Overlay * m_overlay;
  } m_coverageInfo;

  struct CachedCoverageInfo
  {
    CachedCoverageInfo();
    ~CachedCoverageInfo();

    void ResetDL();

    graphics::DisplayList * m_mainElements;
    graphics::DisplayList * m_sharpElements;
    ScreenBase m_screen;
    int m_renderLeafTilesCount;
    bool m_isEmptyDrawing;
  };

  CachedCoverageInfo * m_currentCoverage;
  CachedCoverageInfo * m_backCoverage;

  core::CommandsQueue m_queue;
  shared_ptr<WindowHandle> m_windowHandle;
  shared_ptr<graphics::Screen> m_cacheScreen;
};
