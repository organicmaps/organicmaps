#pragma once

#include "../std/shared_ptr.hpp"
#include "events.hpp"
#include "drawer_yg.hpp"

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

public:

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

  virtual void invalidate() = 0;
};
