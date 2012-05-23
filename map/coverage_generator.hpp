#pragma once

#include "render_policy.hpp"
#include "tiler.hpp"
#include "tile.hpp"
#include "window_handle.hpp"

#include "../geometry/screenbase.hpp"

#include "../yg/overlay.hpp"

#include "../base/thread.hpp"
#include "../base/threaded_list.hpp"
#include "../base/mutex.hpp"
#include "../base/commands_queue.hpp"

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

class TileRenderer;
class TileCache;
class ScreenCoverage;

namespace yg
{
  class Skin;

  namespace gl
  {
    class Screen;
  }
  class ResourceStyleCache;
}

class CoverageGenerator
{
private:

  core::CommandsQueue m_queue;

  TileRenderer * m_tileRenderer;

  shared_ptr<yg::ResourceManager> m_resourceManager;
  shared_ptr<yg::gl::RenderContext> m_renderContext;

  ScreenCoverage * m_workCoverage;
  ScreenCoverage * m_currentCoverage;

  ScreenBase m_currentScreen;
  int m_sequenceID;

  shared_ptr<WindowHandle> m_windowHandle;

  threads::Mutex m_mutex;

  RenderPolicy::TEmptyModelFn m_emptyModelFn;

  yg::gl::PacketsQueue * m_glQueue;
  string m_skinName;

  ScreenCoverage * CreateCoverage();

public:

  CoverageGenerator(string const & skinName,
                    TileRenderer * tileRenderer,
                    shared_ptr<WindowHandle> const & windowHandle,
                    shared_ptr<yg::gl::RenderContext> const & primaryRC,
                    shared_ptr<yg::ResourceManager> const & rm,
                    yg::gl::PacketsQueue * glQueue,
                    RenderPolicy::TEmptyModelFn emptyModelFn);

  ~CoverageGenerator();

  void InitializeThreadGL();
  void FinalizeThreadGL();

  void InvalidateTiles(m2::AnyRectD const & rect, int startScale);
  void InvalidateTilesImpl(m2::AnyRectD const & rect, int startScale);

  void AddCoverScreenTask(ScreenBase const & screen, bool doForce);
  void AddMergeTileTask(Tiler::RectInfo const & rectInfo,
                        int sequenceID);

  void AddCheckEmptyModelTask(int sequenceID);

  void CoverScreen(ScreenBase const & screen, int sequenceID);
  void MergeTile(Tiler::RectInfo const & rectInfo, int sequenceID);
  void CheckEmptyModel(int sequenceID);

  void Cancel();

  void WaitForEmptyAndFinished();

  bool IsEmptyModelAtPoint(m2::PointD const & pt) const;

  ScreenCoverage & CurrentCoverage();

  void SetSequenceID(int sequenceID);

  threads::Mutex & Mutex();

  shared_ptr<yg::ResourceManager> const & resourceManager() const;
};
