#include "../base/SRC_FIRST.hpp"

#include "utils.hpp"
#include "memento.hpp"
#include "vertexbuffer.hpp"
#include "indexbuffer.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/target_os.hpp"

#include "../base/start_mem_debug.hpp"


namespace yg
{
  namespace gl
  {
    namespace utils
    {
      void setupCoordinates(size_t width, size_t height, bool doSwap /*= true*/)
      {
        OGLCHECK(glViewport(0, 0, width, height));

        OGLCHECK(glMatrixMode(GL_MODELVIEW));
        OGLCHECK(glLoadIdentity());

        OGLCHECK(glMatrixMode(GL_PROJECTION));
        OGLCHECK(glLoadIdentity());

        int const bound = 12000;

#ifdef OMIM_GL_ES
        if (!doSwap)
          OGLCHECK(glOrthof(0, width, 0, height, -bound, bound));
        else
          OGLCHECK(glOrthof(0, width, height, 0, -bound, bound));
#else
        if (!doSwap)
          OGLCHECK(glOrtho(0, width, 0, height, -bound, bound));
        else
          OGLCHECK(glOrtho(0, width, height, 0, -bound, bound));
#endif
      }
    }
  }
}
