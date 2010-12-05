#include "../base/SRC_FIRST.hpp"

#include "internal/opengl.hpp"

#include "render_command.hpp"
#include "vertexbuffer.hpp"
#include "indexbuffer.hpp"
#include "vertex.hpp"
#include "base_texture.hpp"
#include "blitter.hpp"

#include "../base/mutex.hpp"

namespace yg
{
  namespace gl
  {
    void UpdateActualTarget::operator()()
    {
    }
  }
}
