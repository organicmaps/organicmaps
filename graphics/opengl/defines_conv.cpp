#include "graphics/opengl/defines_conv.hpp"
#include "base/macros.hpp"

namespace graphics
{
  namespace gl
  {
    struct Info
    {
      int m_firstFull;
      int m_firstBase;
      char const * m_firstName;
      int m_secondFull;
      int m_secondBase;
      int m_countBase;
      char const * m_secondName;
    };

    Info s_dataTypes [] =
    {
      {GL_FLOAT, GL_FLOAT, "GL_FLOAT", EFloat, EFloat, 1, "EFloatVec1"},
      {GL_FLOAT_VEC2, GL_FLOAT, "GL_FLOAT_VEC2", EFloatVec2, EFloat, 2, "EFloatVec2"},
      {GL_FLOAT_VEC3, GL_FLOAT, "GL_FLOAT_VEC3", EFloatVec3, EFloat, 3, "EFloatVec3"},
      {GL_FLOAT_VEC4, GL_FLOAT, "GL_FLOAT_VEC4", EFloatVec4, EFloat, 4, "EFloatVec4"},
      {GL_INT, GL_INT, "GL_INT", EInteger, EInteger, 1, "EIntegerVec1"},
      {GL_INT_VEC2, GL_INT, "GL_INT_VEC2", EIntegerVec2, EInteger, 2, "EIntegerVec2"},
      {GL_INT_VEC3, GL_INT, "GL_INT_VEC3", EIntegerVec3, EInteger, 3, "EIntegerVec3"},
      {GL_INT_VEC4, GL_INT, "GL_INT_VEC4", EIntegerVec4, EInteger, 4, "EIntegerVec4"},
      {GL_FLOAT_MAT2, GL_FLOAT_MAT2, "GL_FLOAT_MAT2", EFloatMat2, EFloatMat2, 1, "EFloatMat2"},
      {GL_FLOAT_MAT3, GL_FLOAT_MAT3, "GL_FLOAT_MAT3", EFloatMat3, EFloatMat3, 1, "EFloatMat3"},
      {GL_FLOAT_MAT4, GL_FLOAT_MAT4, "GL_FLOAT_MAT4", EFloatMat4, EFloatMat4, 1, "EFloatMat4"},
      {GL_SAMPLER_2D, GL_SAMPLER_2D, "GL_SAMPLER_2D", ESampler2D, ESampler2D, 1, "ESampler2D"}
    };

    void convert(GLenum from, EDataType & to)
    {
      for (unsigned i = 0; i < ARRAY_SIZE(s_dataTypes); ++i)
        if (s_dataTypes[i].m_firstFull == from)
        {
          to = (EDataType)s_dataTypes[i].m_secondFull;
          return;
        }
      to = (EDataType)0;
      LOG(LERROR, ("Unknown GLenum for EDataType specified"));
    }

    void convert(EDataType from, GLenum & to)
    {
      for (unsigned i = 0; i < ARRAY_SIZE(s_dataTypes); ++i)
        if (s_dataTypes[i].m_secondFull == from)
        {
          to = s_dataTypes[i].m_firstFull;
          return;
        }

      to = 0;
      LOG(LERROR, ("Unknown EDataType specified"));
    }

    void convert(GLenum from, EDataType & to, size_t & count)
    {
      for (unsigned i = 0; i < ARRAY_SIZE(s_dataTypes); ++i)
        if (s_dataTypes[i].m_firstFull == from)
        {
          to = (EDataType)s_dataTypes[i].m_secondBase;
          count = s_dataTypes[i].m_countBase;
          return;
        }
      to = (EDataType)0;
      count = 0;
      LOG(LERROR, ("Unknown GLenum for EDataType specified"));
    }

    void convert(EShaderType from, GLenum & to)
    {
      switch (from)
      {
      case EVertexShader:
        to = GL_VERTEX_SHADER;
        break;
      case EFragmentShader:
        to = GL_FRAGMENT_SHADER;
        break;
      default:
        LOG(LERROR, ("Unknown EShaderType specified: ", from));
        to = 0;
      }
    }

    void convert(EPrimitives from, GLenum & to)
    {
      switch (from)
      {
      case ETriangles:
        to = GL_TRIANGLES;
        break;
      case ETrianglesFan:
        to = GL_TRIANGLE_FAN;
        break;
      case ETrianglesStrip:
        to = GL_TRIANGLE_STRIP;
        break;
      default:
        LOG(LERROR, ("Unknown EPrimitives specified: ", from));
        to = 0;
      }
    }
  }
}
