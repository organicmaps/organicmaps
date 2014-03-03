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

      m_vxShaders[EVxTextured].reset(new Shader(vxSrc, EVertexShader));

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

      m_vxShaders[EVxSharp].reset(new Shader(sharpVxSrc, EVertexShader));

      /// Fragment Shader with alphaTest

      static const char alphaTestFrgSrc [] =
        "uniform sampler2D Sampler0;\n"
        "varying " PRECISION " vec2 TexCoordOut0;\n"
        "void main(void) {\n"
        "   gl_FragColor = texture2D(Sampler0, TexCoordOut0);\n"
        "   if (gl_FragColor.a == 0.0)\n"
        "     discard;\n"
        "}\n";

      m_frgShaders[EFrgAlphaTest].reset(new Shader(alphaTestFrgSrc, EFragmentShader));

      /// Fragment shader without alphaTest

      static const char noAlphaTestFrgSrc[] =
        "uniform sampler2D Sampler0;\n"
        "varying " PRECISION " vec2 TexCoordOut0;\n"
        "void main(void) {\n"
        "   gl_FragColor = texture2D(Sampler0, TexCoordOut0);\n"
        "}\n";
      m_frgShaders[EFrgNoAlphaTest].reset(new Shader(noAlphaTestFrgSrc, EFragmentShader));

      static const char uniformAlfaFrgSrc[] =
        "uniform sampler2D Sampler0;\n"
        "uniform " PRECISION " float Transparency;\n"
        "varying " PRECISION " vec2 TexCoordOut0;\n"
        "void main(void) {\n"
        " " PRECISION " vec4 color = texture2D(Sampler0, TexCoordOut0);\n"
        " " PRECISION " float t = color.a;\n"
        "  if (t > Transparency)\n"
        "    t = Transparency;\n"
        "  gl_FragColor = vec4(color.rgb, t);\n"
        "}\n";

      m_frgShaders[EFrgVarAlfa].reset(new Shader(uniformAlfaFrgSrc, EFragmentShader));

      getProgram(EVxTextured, EFrgAlphaTest);
      getProgram(EVxTextured, EFrgNoAlphaTest);
      getProgram(EVxTextured, EFrgVarAlfa);

      getProgram(EVxSharp, EFrgAlphaTest);
      getProgram(EVxSharp, EFrgNoAlphaTest);
    }

    shared_ptr<Program> const ProgramManager::getProgram(EVxType vxType,
                                                         EFrgType frgType)
    {
      pair<EVxType, EFrgType> key(vxType, frgType);

      map<pair<EVxType, EFrgType>, shared_ptr<Program> >::const_iterator it = m_programs.find(key);
      if (it != m_programs.end())
        return it->second;

      shared_ptr<Program> program(new Program(m_vxShaders[vxType], m_frgShaders[frgType]));

      m_programs[key] = program;

      return program;
    }
  }
}
