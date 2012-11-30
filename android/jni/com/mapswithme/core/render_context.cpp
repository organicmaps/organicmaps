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
  }

  shared_ptr<graphics::RenderContext> RenderContext::createShared()
  {
    shared_ptr<RenderContext> rc(new RenderContext());
    rc->setResourceManager(resourceManager());
    return rc;
  }
}


