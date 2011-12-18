#pragma once

#include "../base/thread.hpp"
#include "../base/condition.hpp"
#include "../base/commands_queue.hpp"
#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"
#include "../std/list.hpp"
#include "../std/function.hpp"
#include "../yg/color.hpp"
#include "../yg/renderer.hpp"

class DrawerYG;

namespace threads
{
  class Condition;
}

class PaintEvent;
class WindowHandle;

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
    class Screen;
  }
}

class RenderQueueRoutine : public threads::IRoutine
{
public:

  typedef function<void(shared_ptr<PaintEvent>, ScreenBase const &, m2::RectD const &, m2::RectD const &, int)> render_fn_t;
  typedef function<bool (m2::PointD const &)> TEmptyModelFn;

private:

  struct RenderModelCommand
  {
    ScreenBase m_frameScreen;
    shared_ptr<PaintEvent> m_paintEvent;
    render_fn_t m_renderFn;
    RenderModelCommand(ScreenBase const & frameScreen,
                       render_fn_t renderFn);
  };

  struct Invalidate : public yg::gl::Renderer::Command
  {
    list<shared_ptr<WindowHandle> > m_windowHandles;
    void perform();
  };

  shared_ptr<yg::gl::RenderContext> m_renderContext;
  shared_ptr<yg::gl::FrameBuffer> m_frameBuffer;
  shared_ptr<yg::gl::FrameBuffer> m_auxFrameBuffer;
  shared_ptr<DrawerYG> m_threadDrawer;
  shared_ptr<yg::gl::Screen> m_auxScreen;

  shared_ptr<yg::gl::RenderBuffer> m_newDepthBuffer;
  shared_ptr<yg::gl::BaseTexture> m_newActualTarget;
  shared_ptr<yg::gl::BaseTexture> m_newBackBuffer;

  threads::Condition m_hasRenderCommands;
  threads::Condition * m_glCondition;

  shared_ptr<RenderModelCommand> m_currentRenderCommand;
  list<shared_ptr<RenderModelCommand> > m_renderCommands;
  list<shared_ptr<RenderModelCommand> > m_benchmarkRenderCommands;

  shared_ptr<yg::gl::RenderState> m_renderState;

  shared_ptr<yg::ResourceManager> m_resourceManager;

  /// A list of window handles to notify about ending rendering operations.
  list<shared_ptr<WindowHandle> > m_windowHandles;

  bool m_isMultiSampled;
  bool m_doPeriodicalUpdate;
  double m_updateInterval;
  double m_visualScale;
  string m_skinName;
  bool m_isBenchmarking;
  unsigned m_scaleEtalonSize;
  yg::Color m_bgColor;
  ThreadedList<yg::gl::Renderer::Packet> * m_glQueue;

  TEmptyModelFn m_emptyModelFn;

  void waitForRenderCommand(list<shared_ptr<RenderModelCommand> > & cmdList,
                            threads::ConditionGuard & guard);

public:
  RenderQueueRoutine(shared_ptr<yg::gl::RenderState> const & renderState,
                     string const & skinName,
                     bool isMultiSampled,
                     bool doPeriodicalUpdate,
                     double updateInterval,
                     bool isBenchmarking,
                     unsigned scaleEtalonSize,
                     double visualScale,
                     yg::Color const & bgColor);
  /// initialize GL rendering
  /// this function is called just before the thread starts.
  void initializeGL(shared_ptr<yg::gl::RenderContext> const & renderContext,
                    shared_ptr<yg::ResourceManager> const & resourceManager);
  /// This function should always be called from the main thread.
  void Cancel();

  void onSize(int w, int h);

  void SetEmptyModelFn(TEmptyModelFn const checkFn) { m_emptyModelFn = checkFn; }

  /// Check, whether the resize command is queued, and resize accordingly.
  void processResize(ScreenBase const & frameScreen);
  /// Get update areas for the current render state
  bool getUpdateAreas(ScreenBase const & oldScreen, m2::RectI const & oldRect,
                      ScreenBase const & newScreen, m2::RectI const & newRect,
                      vector<m2::RectI> & areas);
  /// Thread procedure
  void Do();
  /// invalidate all connected window handles
  void invalidate();
  /// add monitoring window
  void addWindowHandle(shared_ptr<WindowHandle> window);
  /// add model rendering command to rendering queue
  void addCommand(render_fn_t const & fn, ScreenBase const & frameScreen);
  /// set the resolution scale factor to the main thread drawer;
  void setVisualScale(double visualScale);
  /// free all available memory
  void memoryWarning();
  /// free all easily recreatable opengl resources and make sure that no opengl call will be made.
  void enterBackground();
  /// recreate all necessary opengl resources and prepare to run in foreground.
  void enterForeground();
  /// wait for all commands are processed.
  void waitForEmptyAndFinished();

  void setGLQueue(ThreadedList<yg::gl::Renderer::Packet> * glQueue,
                  threads::Condition * glCondition);
};
