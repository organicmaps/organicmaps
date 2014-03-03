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
    enum EVxType
    {
      EVxTextured,
      EVxSharp
    };

    enum EFrgType
    {
      EFrgAlphaTest,
      EFrgNoAlphaTest,
      EFrgVarAlfa
    };

    class ProgramManager
    {
    private:

      map<EVxType, shared_ptr<Shader> > m_vxShaders;
      map<EFrgType, shared_ptr<Shader> > m_frgShaders;
      map<pair<EVxType, EFrgType> , shared_ptr<Program> > m_programs;

    public:

      ProgramManager();

      shared_ptr<Program> const getProgram(EVxType vxType,
                                           EFrgType frgType);
    };
  }
}
