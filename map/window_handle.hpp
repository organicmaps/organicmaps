#pragma once

#include "../platform/video_timer.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/bind.hpp"
#include "events.hpp"
#include "drawer_yg.hpp"
#include "../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    class RenderContext;
  }
}

class RenderPolicy;

class WindowHandle
{
  shared_ptr<yg::gl::RenderContext> m_renderContext;
  RenderPolicy * m_renderPolicy;

  bool m_hasPendingUpdates;
  bool m_isUpdatesEnabled;
  bool m_needRedraw;

  VideoTimer * m_videoTimer;
  VideoTimer::TFrameFn m_frameFn;
  int m_stallsCount;

public:

  WindowHandle();
  virtual ~WindowHandle();

  void setRenderPolicy(RenderPolicy * renderPolicy);
  void setVideoTimer(VideoTimer * videoTimer);

  void checkedFrameFn();

  bool needRedraw() const;

  void checkTimer();

  void setNeedRedraw(bool flag);

  shared_ptr<yg::gl::RenderContext> const & renderContext();

  void setRenderContext(shared_ptr<yg::gl::RenderContext> const & renderContext);

  bool setUpdatesEnabled(bool doEnable);

  void invalidate();
};
