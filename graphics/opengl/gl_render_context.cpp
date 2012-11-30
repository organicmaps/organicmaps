#include "gl_render_context.hpp"
#include "opengl.hpp"

namespace graphics
{
  namespace gl
  {
    void RenderContext::startThreadDrawing()
    {
      OGLCHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
      OGLCHECK(glPixelStorei(GL_PACK_ALIGNMENT, 1));
    }

    void RenderContext::endThreadDrawing()
    {}
  }
}
