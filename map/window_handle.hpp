#pragma once

#include "../std/shared_ptr.hpp"
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

class WindowHandle
{
  shared_ptr<yg::gl::RenderContext> m_renderContext;

  bool m_hasPendingUpdates;
  bool m_isUpdatesEnabled;
  bool m_needRedraw;

public:
  WindowHandle() :
    m_hasPendingUpdates(false),
    m_isUpdatesEnabled(true),
    m_needRedraw(true)
  {}
  virtual ~WindowHandle() {}

  bool needRedraw() const
  {
    return m_isUpdatesEnabled && m_needRedraw;
  }

  void setNeedRedraw(bool flag)
  {
    m_needRedraw = flag;
  }

  shared_ptr<yg::gl::RenderContext> const & renderContext()
  {
    return m_renderContext;
  }

  void setRenderContext(shared_ptr<yg::gl::RenderContext> const & renderContext)
  {
    m_renderContext = renderContext;
  }

  bool setUpdatesEnabled(bool doEnable)
  {
    bool res = false;
    if ((!m_isUpdatesEnabled) && (doEnable) && (m_hasPendingUpdates))
    {
      setNeedRedraw(true);
      m_hasPendingUpdates = false;
      res = true;
    }
    m_isUpdatesEnabled = doEnable;
    return res;
  }

  void invalidate()
  {
    if (m_isUpdatesEnabled)
      setNeedRedraw(true);
    else
      m_hasPendingUpdates = true;
  }
};
