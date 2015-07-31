#include "graphics/opengl/program_manager.hpp"
#include "graphics/opengl/opengl.hpp"

namespace graphics
{
  namespace gl
  {

#if defined(OMIM_GL_ES)
  #define PRECISION "lowp"
  #define PRECISION_ROUTE "precision highp float;\n"
#else
  #define PRECISION ""
  #define PRECISION_ROUTE ""
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
        "  if (t < 0.05)\n"
        "    discard;\n"
        "  gl_FragColor = vec4(color.rgb, t);\n"
        "}\n";

      m_frgShaders[EFrgVarAlfa].reset(new Shader(uniformAlfaFrgSrc, EFragmentShader));

      static const char routeVxSrc[] =
        PRECISION_ROUTE
        "attribute vec3 Position;\n"
        "attribute vec2 Normal;\n"
        "attribute vec3 Length;\n"
        "uniform mat4 ModelView;\n"
        "uniform mat4 Projection;\n"
        "uniform vec2 u_halfWidth;\n"
        "varying vec2 v_length;\n"
        "void main(void)\n"
        "{\n"
        "  float normalLen = length(Normal);\n"
        "  vec2 transformedAxisPos = (vec4(Position.xy, 0.0, 1.0) * ModelView).xy;\n"
        "  vec2 len = vec2(Length.x, Length.z);\n"
        "  if (u_halfWidth.x != 0.0 && normalLen != 0.0)\n"
        "  {\n"
        "    vec2 norm = Normal * u_halfWidth.x;\n"
        "    float actualHalfWidth = length(norm);\n"
        "    vec4 glbShiftPos = vec4(Position.xy + norm, 0.0, 1.0);\n"
        "    vec2 shiftPos = (glbShiftPos * ModelView).xy;\n"
        "    transformedAxisPos = transformedAxisPos + normalize(shiftPos - transformedAxisPos) * actualHalfWidth;\n"
        "    if (u_halfWidth.y != 0.0)\n"
        "      len = vec2(Length.x + Length.y * u_halfWidth.y, Length.z);\n"
        "  }\n"
        "  v_length = len;\n"
        "  gl_Position = vec4(transformedAxisPos, 0.0, 1.0) * Projection;\n"
        "}\n";

      m_vxShaders[EVxRoute].reset(new Shader(routeVxSrc, EVertexShader));

      static const char routeFrgSrc[] =
        PRECISION_ROUTE
        "varying vec2 v_length;\n"
        "uniform vec4 u_color;\n"
        "uniform float u_clipLength;\n"
        "void main(void)\n"
        "{\n"
        "  vec4 color = u_color;\n"
        "  if (v_length.x < u_clipLength)\n"
        "    color.a = 0.0;\n"
        "  gl_FragColor = color;\n"
        "}\n";

      m_frgShaders[EFrgRoute].reset(new Shader(routeFrgSrc, EFragmentShader));

      static const char routeArrowFrgSrc[] =
        PRECISION_ROUTE
        "varying vec2 v_length;\n"
        "uniform sampler2D Sampler0;\n"
        "uniform vec4 u_textureRect;\n"
        "void main(void)\n"
        "{\n"
        "  float u = clamp(v_length.x, 0.0, 1.0);\n"
        "  float v = 0.5 * v_length.y + 0.5;\n"
        "  vec2 uv = vec2(mix(u_textureRect.x, u_textureRect.z, u), mix(u_textureRect.y, u_textureRect.w, v));\n"
        "  vec4 color = texture2D(Sampler0, uv);\n"
        "  gl_FragColor = color;\n"
        "}\n";

      m_frgShaders[EFrgRouteArrow].reset(new Shader(routeArrowFrgSrc, EFragmentShader));

      getProgram(EVxTextured, EFrgAlphaTest);
      getProgram(EVxTextured, EFrgNoAlphaTest);
      getProgram(EVxTextured, EFrgVarAlfa);

      getProgram(EVxSharp, EFrgAlphaTest);
      getProgram(EVxSharp, EFrgNoAlphaTest);

      getProgram(EVxRoute, EFrgRoute);
      getProgram(EVxRoute, EFrgRouteArrow);
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
