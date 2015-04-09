#pragma once

#include "graphics/opengl/gl_render_context.hpp"

namespace srv
{
  class RenderContext : public graphics::gl::RenderContext
  {
  public:
    RenderContext();

    virtual void makeCurrent();
    virtual RenderContext * createShared();
  };
}
