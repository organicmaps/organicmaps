#pragma once

#include "../base/thread.hpp"
#include "../geometry/screenbase.hpp"
#include "../std/shared_ptr.hpp"
#include "render_queue_routine.hpp"
#include "../yg/tile_cache.hpp"
#include "../yg/tiler.hpp"
#include "../base/threaded_list.hpp"

namespace yg
{
  class ResourceManager;
  namespace gl
  {
    class RenderState;
    class RenderContext;
  }
}

class WindowHandle;

class RenderQueue
{
private:

  /// single rendering task
  struct Task
  {
    threads::Thread m_thread;
    RenderQueueRoutine * m_routine;
  };

  Task * m_tasks;
  size_t m_tasksCount;

  size_t m_sequence;

  friend class RenderQueueRoutine;

  shared_ptr<yg::ResourceManager> m_resourceManager;

  ThreadedList<shared_ptr<RenderQueueRoutine::Command> > m_renderCommands;

  yg::TileCache m_tileCache;

public:

  typedef RenderQueueRoutine::renderCommandFinishedFn renderCommandFinishedFn;

  /// constructor.
  RenderQueue(string const & skinName,
              bool isBenchmarking,
              unsigned scaleEtalonSize,
              unsigned maxTilesCount,
              unsigned tasksCount,
              yg::Color const & bgColor);
  /// destructor.
  ~RenderQueue();
  /// set the primary context. it starts the rendering thread.
  void InitializeGL(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                    shared_ptr<yg::ResourceManager> const & resourceManager,
                    double ppmScale);
  /// add command to the commands queue.
  void AddCommand(RenderQueueRoutine::render_fn_t const & fn, yg::Tiler::RectInfo const & rectInfo, size_t seqNum);
  /// set visual scale
  void SetVisualScale(double visualScale);
  /// add window handle to notify when rendering operation finishes
  void AddWindowHandle(shared_ptr<WindowHandle> const & windowHandle);
  /// add function, that will be called upon render command completion.
  void AddRenderCommandFinishedFn(renderCommandFinishedFn fn);
  /// free all possible memory caches.
  void MemoryWarning();
  /// free all possible memory caches, opengl resources,
  /// and make sure no opengl call will be made in background
  void EnterBackground();
  /// load all necessary memory caches and opengl resources.
  void EnterForeground();
  /// get tile cache.
  yg::TileCache & TileCache();
  /// number of the current tiler sequence to skip the old commands upon rendering.
  size_t CurrentSequence() const;
  /// common render commands queue for all rendering threads.
  ThreadedList<shared_ptr<RenderQueueRoutine::Command> > & RenderCommands();
};
