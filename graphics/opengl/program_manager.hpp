#pragma once

#include "program.hpp"
#include "shader.hpp"

#include "../../std/shared_ptr.hpp"
#include "../../std/map.hpp"
#include "../../std/string.hpp"

namespace graphics
{
  namespace gl
  {
    class ProgramManager
    {
    private:

      map<string, shared_ptr<Shader> > m_vxShaders;
      map<string, shared_ptr<Shader> > m_frgShaders;
      map<string, shared_ptr<Program> > m_programs;

    public:

      ProgramManager();

      shared_ptr<Program> const getProgram(char const * vxName,
                                           char const * frgName);
    };
  }
}
