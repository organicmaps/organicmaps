/*
 * render_context.hpp
 *
 *  Created on: Oct 14, 2011
 *      Author: siarheirachytski
 */

#pragma once

#include "../../../../../yg/rendercontext.hpp"

#include "../../../../../std/shared_ptr.hpp"

namespace android
{
  class RenderContext : public yg::gl::RenderContext
  {
  public:
    RenderContext();

    virtual void makeCurrent();

    virtual shared_ptr<yg::gl::RenderContext> createShared();

    virtual void endThreadDrawing();
  };
}
