#pragma once

#include "render_policy.hpp"
#include "tiler.hpp"
#include "tile.hpp"
#include "window_handle.hpp"

#include "../geometry/screenbase.hpp"

#include "../graphics/overlay.hpp"

#include "../base/thread.hpp"
#include "../base/threaded_list.hpp"
#include "../base/mutex.hpp"
#include "../base/commands_queue.hpp"

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

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
  ScreenCoverage * CreateCoverage();

public:

  CoverageGenerator(TileRenderer * tileRenderer,
                    shared_ptr<WindowHandle> const & windowHandle,
                    shared_ptr<graphics::RenderContext> const & primaryRC,
                    shared_ptr<graphics::ResourceManager> const & rm,
                    graphics::PacketsQueue * glQueue,
                    RenderPolicy::TCountryIndexFn const & countryIndexFn);

  ~CoverageGenerator();

  void Shutdown();

  void InitializeThreadGL();
  void FinalizeThreadGL();

  //@{ Add task to run on CoverageGenerator thread
  void InvalidateTiles(m2::AnyRectD const & rect, int startScale);
  void CoverScreen(ScreenBase const & screen, bool doForce);
  void MergeTile(Tiler::RectInfo const & rectInfo,
                        int sequenceID);
  void FinishSequenceIfNeeded();
  void DecrementTileCount(int sequenceID);
  void CheckEmptyModel(int sequenceID);
  //}@

  //@{ Benchmark support
  int InsertBenchmarkFence();
  void JoinBenchmarkFence(int fenceID);
  //}@

  storage::TIndex GetCountryIndex(m2::PointD const & pt) const;

  ScreenCoverage * CurrentCoverage();

  void StartTileDrawingSession(int sequenceID, unsigned tileCount);

  //@{ Frame lock
  void Lock();
  void Unlock();
  //}@

  void Pause();
  void Resume();

  bool DoForceUpdate() const;

private:
  void CoverScreenImpl(core::CommandsQueue::Environment const & env,
                       ScreenBase const & screen,
                       int sequenceID);

  void MergeTileImpl(core::CommandsQueue::Environment const & env,
                     Tiler::RectInfo const & rectInfo,
                     int sequenceID);

  void InvalidateTilesImpl(m2::AnyRectD const & rect, int startScale);

  void CheckEmptyModelImpl(int sequenceID);

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

  core::CommandsQueue m_queue;

  TileRenderer * m_tileRenderer;

  shared_ptr<graphics::ResourceManager> m_resourceManager;
  shared_ptr<graphics::RenderContext> m_renderContext;

  ScreenCoverage * m_workCoverage;
  ScreenCoverage * m_currentCoverage;

  shared_ptr<WindowHandle> m_windowHandle;

  RenderPolicy::TCountryIndexFn m_countryIndexFn;

  graphics::PacketsQueue * m_glQueue;
};
