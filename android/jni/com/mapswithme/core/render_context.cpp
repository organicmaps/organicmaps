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

  graphics::RenderContext * RenderContext::createShared()
  {
    RenderContext * rc = new RenderContext();
    rc->setResourceManager(resourceManager());
    return rc;
  }
}


