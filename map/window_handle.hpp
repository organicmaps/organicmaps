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
private:

  shared_ptr<DrawerYG> m_drawer;
  shared_ptr<yg::gl::RenderContext> m_renderContext;

  bool m_hasPendingUpdates;
  bool m_isUpdatesEnabled;

public:

  WindowHandle() : m_hasPendingUpdates(false), m_isUpdatesEnabled(true)
  {}

  shared_ptr<DrawerYG> const & drawer()
  {
    return m_drawer;
  }

  shared_ptr<yg::gl::RenderContext> const & renderContext()
  {
    return m_renderContext;
  }

  void setRenderContext(shared_ptr<yg::gl::RenderContext> const & renderContext)
  {
    m_renderContext = renderContext;
  }

  void setDrawer(shared_ptr<DrawerYG> const & drawer)
  {
    m_drawer = drawer;
  }

  bool setUpdatesEnabled(bool doEnable)
  {
    bool res = false;
    if ((!m_isUpdatesEnabled) && (doEnable) && (m_hasPendingUpdates))
    {
      invalidateImpl();
      m_hasPendingUpdates = false;
      res = true;
    }
    m_isUpdatesEnabled = doEnable;
    return res;
  }

  void invalidate()
  {
    if (m_isUpdatesEnabled)
      invalidateImpl();
    else
      m_hasPendingUpdates = true;
  }

  virtual void invalidateImpl() = 0;
};
