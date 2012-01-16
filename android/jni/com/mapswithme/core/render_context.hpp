#pragma once

#include "../../../../../yg/rendercontext.hpp"

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
