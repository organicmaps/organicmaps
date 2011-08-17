#pragma once

#include "../base/thread.hpp"
#include "../base/condition.hpp"
#include "../base/commands_queue.hpp"
#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"
#include "../std/list.hpp"
#include "../std/function.hpp"
#include "../yg/color.hpp"
#include "tile_cache.hpp"
#include "tiler.hpp"
#include "render_policy.hpp"

class DrawerYG;

namespace threads
{
  class Condition;
}

class PaintEvent;
class WindowHandle;
class RenderQueue;

namespace yg
{
  class ResourceManager;


  namespace gl
  {
    class RenderContext;
    class FrameBuffer;
    class RenderBuffer;
    class BaseTexture;
    class RenderState;
    class RenderState;
    class Screen;
  }
}

class RenderQueueRoutine : public threads::IRoutine
{
public:

  typedef RenderPolicy::TRenderFn TRenderFn;
  typedef function<void(Tiler::RectInfo const &, Tile const &)> TCommandFinishedFn;

  /// Single tile rendering command
  struct Command
  {
    TRenderFn m_renderFn;
    Tiler::RectInfo m_rectInfo;
    shared_ptr<PaintEvent> m_paintEvent; //< paintEvent is set later after construction
    size_t m_sequenceID;
    TCommandFinishedFn m_commandFinishedFn;
    Command(TRenderFn renderFn,
            Tiler::RectInfo const & rectInfo,
            size_t sequenceID,
            TCommandFinishedFn commandFinishedFn);
  };

private:

  shared_ptr<yg::gl::RenderContext> m_renderContext;
  shared_ptr<yg::gl::FrameBuffer> m_frameBuffer;
  shared_ptr<DrawerYG> m_threadDrawer;

  threads::Mutex m_mutex;
  shared_ptr<Command> m_currentCommand;

  shared_ptr<yg::ResourceManager> m_resourceManager;


  double m_visualScale;
  string m_skinName;
  unsigned m_scaleEtalonSize;
  yg::Color m_bgColor;

  size_t m_threadNum;

  RenderQueue * m_renderQueue;

  bool HasTile(Tiler::RectInfo const & rectInfo);
  void AddTile(Tiler::RectInfo const & rectInfo, Tile const & tile);

public:

  RenderQueueRoutine(string const & skinName,
                     unsigned scaleEtalonSize,
                     yg::Color const & bgColor,
                     size_t threadNum,
                     RenderQueue * renderQueue);
  /// initialize GL rendering
  /// this function is called just before the thread starts.
  void InitializeGL(shared_ptr<yg::gl::RenderContext> const & renderContext,
                    shared_ptr<yg::ResourceManager> const & resourceManager,
                    double visualScale);
  /// This function should always be called from the main thread.
  void Cancel();
  /// Thread procedure
  void Do();
  /// add model rendering command to rendering queue
  void AddCommand(TRenderFn const & fn, Tiler::RectInfo const & rectInfo, size_t seqNumber);
  /// free all available memory
  void MemoryWarning();
  /// free all easily recreatable opengl resources and make sure that no opengl call will be made.
  void EnterBackground();
  /// recreate all necessary opengl resources and prepare to run in foreground.
  void EnterForeground();
};
