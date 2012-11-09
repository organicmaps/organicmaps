/*
 * render_context.hpp
 *
 *  Created on: Oct 14, 2011
 *      Author: siarheirachytski
 */

#pragma once

#include "../../../../../graphics/rendercontext.hpp"

#include "../../../../../std/shared_ptr.hpp"

namespace android
{
  class RenderContext : public graphics::gl::RenderContext
  {
  public:
    RenderContext();

    virtual void makeCurrent();

    virtual shared_ptr<graphics::gl::RenderContext> createShared();

    virtual void endThreadDrawing();
  };
}
