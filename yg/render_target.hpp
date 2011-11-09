#pragma once

namespace yg
{
  namespace gl
  {
    class RenderTarget
    {
    public:
      virtual ~RenderTarget() {}
      /// attach render target to framebuffer and setup coordinate system
      virtual unsigned int id() const = 0;
      virtual void attachToFrameBuffer() = 0;
      virtual unsigned  width() const = 0;
      virtual unsigned height() const = 0;
    };
  }
}
