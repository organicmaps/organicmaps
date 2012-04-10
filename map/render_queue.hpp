#pragma once

#include "../base/thread.hpp"
#include "../base/threaded_list.hpp"
#include "../yg/renderer.hpp"
#include "../geometry/screenbase.hpp"
#include "../std/shared_ptr.hpp"
#include "render_queue_routine.hpp"

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

  friend class RenderQueueRoutine;

  threads::Thread m_renderQueueThread;

  shared_ptr<yg::gl::RenderState> m_renderState;
  shared_ptr<yg::ResourceManager> m_resourceManager;
  RenderQueueRoutine * m_routine;
  bool m_hasPendingResize;
  int m_savedWidth;
  int m_savedHeight;

public:
  /// constructor.
  RenderQueue(string const & skinName,
              bool isMultiSampled,
              bool doPeriodicalUpdate,
              double updateInterval,
              bool isBenchmarking,
              unsigned scaleEtalonSize,
              double visualScale,
              yg::Color const & bgColor);
  /// destructor.
  ~RenderQueue();
  /// set the primary context. it starts the rendering thread.
  void initializeGL(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                    shared_ptr<yg::ResourceManager> const & resourceManager);
  /// add command to the commands queue.
  void AddCommand(RenderQueueRoutine::render_fn_t const & fn, ScreenBase const & frameScreen);

  void SetRedrawAll();

  void SetEmptyModelFn(RenderQueueRoutine::TEmptyModelFn fn);

  void SetVisualScale(double visualScale);

  /// add window handle to notify when rendering operation finishes
  void AddWindowHandle(shared_ptr<WindowHandle> const & windowHandle);
  /// process resize request
  void OnSize(size_t w, size_t h);
  /// copy primary render state
  yg::gl::RenderState const CopyState() const;

  shared_ptr<yg::gl::RenderState> const & renderStatePtr() const;

  yg::gl::RenderState const & renderState() const;

  /// free all possible memory caches
  void memoryWarning();
  /// free all possible memory caches, opengl resources,
  /// and make sure no opengl call will be made in background
  void enterBackground();
  /// load all necessary memory caches and opengl resources.
  void enterForeground();

  void WaitForEmptyAndFinished();

  void SetGLQueue(yg::gl::PacketsQueue * glQueue);
};
