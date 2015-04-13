/*
 * render_context.hpp
 *
 *  Created on: Oct 14, 2011
 *      Author: siarheirachytski
 */

#pragma once

#include "graphics/opengl/gl_render_context.hpp"

#include "std/shared_ptr.hpp"

namespace android
{
  class RenderContext : public graphics::gl::RenderContext
  {
  public:

    void makeCurrent();

    graphics::RenderContext * createShared();
  };
}
