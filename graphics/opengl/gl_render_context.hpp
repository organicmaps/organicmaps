#pragma once

#include "../../std/shared_ptr.hpp"
#include "../render_context.hpp"
#include "program_manager.hpp"
#include "storage.hpp"

namespace graphics
{
  class ResourceManager;

  namespace gl
  {
    class ProgramManager;
    class Program;

    class RenderContext : public graphics::RenderContext
    {
    private:
      /// Program manager to get Program's from.
      ProgramManager * m_programManager;

      /// @{ OpenGL specific rendering states
      /// Current Storage
      Storage m_storage;
      /// Current Program
      shared_ptr<Program> m_program;
      /// @}
    public:

      ProgramManager * programManager();

      /// @{ OpenGL specific rendering states

      Storage const & storage() const;
      void setStorage(Storage const & storage);

      shared_ptr<Program> const & program() const;
      void setProgram(shared_ptr<Program> const & prg);

      /// @}

      /// Start rendering in the specified thread
      void startThreadDrawing(unsigned threadSlot);
      /// End rendering in the specified thread
      void endThreadDrawing(unsigned threadSlot);
    };
  }
}
