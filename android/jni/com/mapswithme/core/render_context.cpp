/*
 * render_context.cpp
 *
 *  Created on: Oct 14, 2011
 *      Author: siarheirachytski
 */

#include "render_context.hpp"

namespace android
{
  void RenderContext::makeCurrent()
  {
    startThreadDrawing();
  }

  shared_ptr<graphics::RenderContext> RenderContext::createShared()
  {
    return make_shared_ptr(new RenderContext());
  }
}


