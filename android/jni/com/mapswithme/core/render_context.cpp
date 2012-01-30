/*
 * render_context.cpp
 *
 *  Created on: Oct 14, 2011
 *      Author: siarheirachytski
 */

#include "render_context.hpp"

namespace android
{
  RenderContext::RenderContext()
  {}

  void RenderContext::makeCurrent()
  {}

  shared_ptr<yg::gl::RenderContext> RenderContext::createShared()
  {
    return make_shared_ptr(new RenderContext());
  }

  void RenderContext::endThreadDrawing()
  {
  }
}


