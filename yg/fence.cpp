#include "../base/SRC_FIRST.hpp"
#include "fence.hpp"
#include "internal/opengl.hpp"
#include "../base/timer.hpp"
#include "../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    Fence::Fence() : m_inserted(false)
    {
#if defined(OMIM_OS_MAC) && !defined(OMIM_GL_ES)
      OGLCHECK(glGenFencesAPPLE(1, &m_id));
#endif
    }

    Fence::~Fence()
    {
#if defined(OMIM_OS_MAC) && !defined(OMIM_GL_ES)
      OGLCHECK(glDeleteFencesAPPLE(1, &m_id));
#endif
    }

    void Fence::insert()
    {
      if (!m_inserted)
      {
#if defined(OMIM_OS_MAC) && !defined(OMIM_GL_ES)
        OGLCHECK(glSetFenceAPPLE(m_id));
#endif
        m_inserted = true;
      }
    }

    bool Fence::status()
    {
#if defined(OMIM_OS_MAC) && !defined(OMIM_GL_ES)
      return glTestFenceAPPLE(m_id);
#else
      return false;
#endif
    }

    void Fence::wait()
    {
      if (m_inserted)
      {
#if defined(OMIM_OS_MAC) && !defined(OMIM_GL_ES)
        OGLCHECK(glFinishFenceAPPLE(m_id));
#endif
        m_inserted = false;
      }
    }
  }
}
