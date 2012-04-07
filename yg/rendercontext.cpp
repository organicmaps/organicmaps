#include "../base/SRC_FIRST.hpp"
#include "rendercontext.hpp"
#include "internal/opengl.hpp"

namespace yg
{
  namespace gl
  {
    void RenderContext::initParams()
    {
      OGLCHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
      OGLCHECK(glPixelStorei(GL_PACK_ALIGNMENT, 1));
      yg::gl::InitializeThread();
    }

    void RenderContext::endThreadDrawing()
    {
      yg::gl::FinalizeThread();
    }
  }
}
