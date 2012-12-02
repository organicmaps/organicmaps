#include "program_manager.hpp"
#include "opengl.hpp"

namespace graphics
{
  namespace gl
  {
#if defined(OMIM_GL_ES)
  #define PRECISION "lowp"
#else
  #define PRECISION ""
#endif

    ProgramManager::ProgramManager()
    {
      /// Vertex Shader Source
      static const char vxSrc[] =
        "attribute vec4 Position;\n"
        "attribute vec2 Normal;\n"
        "attribute vec2 TexCoord0;\n"
        "uniform mat4 Projection;\n"
        "uniform mat4 ModelView;\n"
        "varying vec2 TexCoordOut0;\n"
        "void main(void) {\n"
        "   gl_Position = (vec4(Normal, 0.0, 0.0) + Position * ModelView) * Projection;\n"
        "   TexCoordOut0 = TexCoord0;\n"
        "}\n";

      m_vxShaders["basic"].reset(new Shader(vxSrc, EVertexShader));

      /// Sharp Vertex Shader Source

      static const char sharpVxSrc[] =
        "attribute vec4 Position;\n"
        "attribute vec2 Normal;\n"
        "attribute vec2 TexCoord0;\n"
        "uniform mat4 Projection;\n"
        "uniform mat4 ModelView;\n"
        "varying vec2 TexCoordOut0;\n"
        "void main(void) {\n"
        "   gl_Position = floor(vec4(Normal, 0.0, 0.0) + Position * ModelView) * Projection;\n"
        "   TexCoordOut0 = TexCoord0;\n"
        "}\n";

      m_vxShaders["sharp"].reset(new Shader(sharpVxSrc, EVertexShader));

      /// Fragment Shader with alphaTest

      static const char alphaTestFrgSrc [] =
        "uniform sampler2D Sampler0;\n"
        "varying " PRECISION " vec2 TexCoordOut0;\n"
        "void main(void) {\n"
        "   gl_FragColor = texture2D(Sampler0, TexCoordOut0);\n"
        "   if (gl_FragColor.a == 0.0)\n"
        "     discard;\n"
        "}\n";

      m_frgShaders["alphatest"].reset(new Shader(alphaTestFrgSrc, EFragmentShader));

      /// Fragment shader without alphaTest

      static const char noAlphaTestFrgSrc[] =
        "uniform sampler2D Sampler0;\n"
        "varying " PRECISION " vec2 TexCoordOut0;\n"
        "void main(void) {\n"
        "   gl_FragColor = texture2D(Sampler0, TexCoordOut0);\n"
        "}\n";
      m_frgShaders["noalphatest"].reset(new Shader(noAlphaTestFrgSrc, EFragmentShader));

      getProgram("basic", "alphatest");
      getProgram("basic", "noalphatest");

      getProgram("sharp", "alphatest");
      getProgram("sharp", "noalphatest");
    }

    shared_ptr<Program> const ProgramManager::getProgram(char const * vxName,
                                                         char const * frgName)
    {
      string prgName(string(vxName) + ":" + frgName);

      map<string, shared_ptr<Program> >::const_iterator it = m_programs.find(prgName);
      if (it != m_programs.end())
        return it->second;

      shared_ptr<Program> program(new Program(m_vxShaders[vxName], m_frgShaders[frgName]));

      m_programs[prgName] = program;

      LOG(LINFO, (this, ", ", vxName, ", ", frgName, ", ", program));

      return program;
    }
  }
}
