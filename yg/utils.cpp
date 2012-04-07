#include "defines.hpp"
#include "utils.hpp"

#include "internal/opengl.hpp"

#include "../std/target_os.hpp"

namespace yg
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
          OGLCHECK(glOrthoFn(0, width, 0, height, -yg::maxDepth, yg::maxDepth));
        else
          OGLCHECK(glOrthoFn(0, width, height, 0, -yg::maxDepth, yg::maxDepth));
      }
    }
  }
}
