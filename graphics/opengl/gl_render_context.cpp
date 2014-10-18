#include "../resource_manager.hpp"

#include "gl_render_context.hpp"
#include "opengl.hpp"

namespace graphics
{
  namespace gl
  {
    void RenderContext::startThreadDrawing(unsigned ts)
    {
      OGLCHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
      OGLCHECK(glPixelStorei(GL_PACK_ALIGNMENT, 1));
      graphics::RenderContext::startThreadDrawing(ts);

      m_programManager = resourceManager()->programManager(threadSlot());
    }

    void RenderContext::endThreadDrawing(unsigned ts)
    {
      graphics::RenderContext::endThreadDrawing(ts);
    }

    Storage const & RenderContext::storage() const
    {
      return m_storage;
    }

    void RenderContext::setStorage(Storage const & storage)
    {
      m_storage = storage;
    }

    shared_ptr<Program> const & RenderContext::program() const
    {
      return m_program;
    }

    void RenderContext::setProgram(shared_ptr<Program> const & prg)
    {
      if (m_program != prg)
      {
        m_program = prg;
        m_program->riseChangedFlag();
      }
    }

    ProgramManager * RenderContext::programManager()
    {
      return m_programManager;
    }
  }
}
