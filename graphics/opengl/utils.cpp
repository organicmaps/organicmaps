#include "graphics/opengl/defines.hpp"
#include "graphics/opengl/utils.hpp"

#include "graphics/opengl/opengl/opengl.hpp"

#include "graphics/std/target_os.hpp"

namespace graphics
{
  namespace gl
  {
    namespace utils
    {
      void setupCoordinates(size_t width, size_t height, bool doSwap /*= true*/)
      {
        OGLCHECK(glViewport(0, 0, width, height));

        OGLCHECK(glMatrixModeFn(GL_MODELVIEW_MWM));
        OGLCHECK(glLoadIdentityFn());

        OGLCHECK(glMatrixModeFn(GL_PROJECTION_MWM));
        OGLCHECK(glLoadIdentityFn());

        if (!doSwap)
          OGLCHECK(glOrthoFn(0, width, 0, height, -graphics::maxDepth, graphics::maxDepth));
        else
          OGLCHECK(glOrthoFn(0, width, height, 0, -graphics::maxDepth, graphics::maxDepth));
      }
    }
  }
}
