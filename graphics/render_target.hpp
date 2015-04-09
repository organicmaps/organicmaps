#pragma once

#include "base/matrix.hpp"

namespace graphics
{
  class RenderTarget
  {
  public:
    virtual ~RenderTarget();
    virtual unsigned int id() const = 0;
    /// attach render target to framebuffer and setup coordinate system
    virtual void attachToFrameBuffer() = 0;
    virtual void detachFromFrameBuffer() = 0;
    virtual void coordMatrix(math::Matrix<float, 4, 4> & m);
    virtual unsigned  width() const = 0;
    virtual unsigned height() const = 0;
  };
}
