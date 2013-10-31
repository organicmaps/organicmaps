#include "shader_def.hpp"

#include "../std/utility.hpp"

namespace gpu
{
  static const char SIMPLE_VERTEX_SHADER_VSH[] = " \
    attribute mediump vec2 position; \
    attribute mediump float depth; \
     \
    uniform mediump mat4 modelViewProjectionMatrix; \
     \
    void main(void) \
    { \
        gl_Position = modelViewProjectionMatrix * vec4(position, depth, 1.0); \
    } \
  ";

  static const char SOLID_AREA_FRAGMENT_SHADER_FSH[] = " \
    uniform lowp vec4 color; \
     \
    void main(void) \
    { \
        gl_FragColor = color; \
    } \
  ";

  static const char TEXTURING_FRAGMENT_SHADER_FSH[] = " \
    uniform sampler2D textureUnit; \
    varying highp vec4 varTexCoords; \
     \
    void main(void) \
    { \
        gl_FragColor = texture2D(textureUnit, varTexCoords.st); \
    } \
  ";

  static const char TEXTURING_VERTEX_SHADER_VSH[] = " \
    attribute mediump vec2 position; \
    attribute mediump float depth; \
    attribute mediump vec4 texCoords; \
     \
    uniform highp mat4 modelViewProjectionMatrix; \
     \
    varying highp vec4 varTexCoords; \
     \
    void main(void) \
    { \
        gl_Position = modelViewProjectionMatrix * vec4(position, depth, 1.0); \
        varTexCoords = texCoords; \
    } \
  ";

  //---------------------------------------------//
  #define SIMPLE_VERTEX_SHADER_VSH_INDEX 0
  #define SOLID_AREA_FRAGMENT_SHADER_FSH_INDEX 1
  #define TEXTURING_FRAGMENT_SHADER_FSH_INDEX 2
  #define TEXTURING_VERTEX_SHADER_VSH_INDEX 3
  //---------------------------------------------//
  const int SOLID_AREA_PROGRAM = 1;
  const int TEXTURING_PROGRAM = 0;
  //---------------------------------------------//

  ProgramInfo::ProgramInfo()
    : m_vertexIndex(-1)
    , m_fragmentIndex(-1)
    , m_vertexSource(NULL)
    , m_fragmentSource(NULL) {}

  ProgramInfo::ProgramInfo(int vertexIndex, int fragmentIndex,
                           const char * vertexSource, const char * fragmentSource)
    : m_vertexIndex(vertexIndex)
    , m_fragmentIndex(fragmentIndex)
    , m_vertexSource(vertexSource)
    , m_fragmentSource(fragmentSource)
  {
  }

  void InitGpuProgramsLib(map<int, ProgramInfo> & gpuIndex)
  {
    gpuIndex.insert(make_pair(TEXTURING_PROGRAM, ProgramInfo(TEXTURING_VERTEX_SHADER_VSH_INDEX, TEXTURING_FRAGMENT_SHADER_FSH_INDEX, TEXTURING_VERTEX_SHADER_VSH, TEXTURING_FRAGMENT_SHADER_FSH)));
    gpuIndex.insert(make_pair(SOLID_AREA_PROGRAM, ProgramInfo(SIMPLE_VERTEX_SHADER_VSH_INDEX, SOLID_AREA_FRAGMENT_SHADER_FSH_INDEX, SIMPLE_VERTEX_SHADER_VSH, SOLID_AREA_FRAGMENT_SHADER_FSH)));
  }
}
