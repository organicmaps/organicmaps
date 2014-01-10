#include "uniform_value.hpp"
#include "glfunctions.hpp"
#include "../base/assert.hpp"
#include "../std/memcpy.hpp"

namespace
{
  template<typename T>
  void CopyValues(T * dstPointer, T * values, size_t valuesCount)
  {
    for (size_t i = 0; i < valuesCount; ++i)
      dstPointer[i] = values[i];
  }

  void ApplyInt(int8_t location, const int32_t * pointer, size_t componentCount)
  {
    switch (componentCount)
    {
    case 1:
      GLFunctions::glUniformValuei(location, pointer[0]);
      break;
    case 2:
      GLFunctions::glUniformValuei(location, pointer[0], pointer[1]);
      break;
    case 3:
      GLFunctions::glUniformValuei(location, pointer[0], pointer[1], pointer[2]);
      break;
    case 4:
      GLFunctions::glUniformValuei(location, pointer[0], pointer[1], pointer[2], pointer[3]);
      break;
    default:
      ASSERT(false, ());
    }
  }

  void ApplyFloat(int8_t location, const float * pointer, size_t componentCount)
  {
    switch (componentCount)
    {
    case 1:
      GLFunctions::glUniformValuef(location, pointer[0]);
      break;
    case 2:
      GLFunctions::glUniformValuef(location, pointer[0], pointer[1]);
      break;
    case 3:
      GLFunctions::glUniformValuef(location, pointer[0], pointer[1], pointer[2]);
      break;
    case 4:
      GLFunctions::glUniformValuef(location, pointer[0], pointer[1], pointer[2], pointer[3]);
      break;
    default:
      ASSERT(false, ());
    }
  }

  void ApplyMatrix(uint8_t location, const float * matrix)
  {
    GLFunctions::glUniformMatrix4x4Value(location, matrix);
  }
}

UniformValue::UniformValue(const string & name, int32_t v)
  : m_name(name)
  , m_type(Int)
  , m_componentCount(1)
{
  Allocate(sizeof(int32_t));
  SetIntValue(v);
}

UniformValue::UniformValue(const string & name, int32_t v1, int32_t v2)
  : m_name(name)
  , m_type(Int)
  , m_componentCount(2)
{
  Allocate(2 * sizeof(int32_t));
  SetIntValue(v1, v2);
}

UniformValue::UniformValue(const string & name, int32_t v1, int32_t v2, int32_t v3)
  : m_name(name)
  , m_type(Int)
  , m_componentCount(3)
{
  Allocate(3 * sizeof(int32_t));
  SetIntValue(v1, v2, v3);
}

UniformValue::UniformValue(const string & name, int32_t v1, int32_t v2, int32_t v3, int32_t v4)
  : m_name(name)
  , m_type(Int)
  , m_componentCount(4)
{
  Allocate(4 * sizeof(int32_t));
  SetIntValue(v1, v2, v3, v4);
}

UniformValue::UniformValue(const string & name, float v)
  : m_name(name)
  , m_type(Float)
  , m_componentCount(1)
{
  Allocate(sizeof(float));
  SetFloatValue(v);
}

UniformValue::UniformValue(const string & name, float v1, float v2)
  : m_name(name)
  , m_type(Float)
  , m_componentCount(2)
{
  Allocate(2 * sizeof(float));
  SetFloatValue(v1, v2);
}

UniformValue::UniformValue(const string & name, float v1, float v2, float v3)
  : m_name(name)
  , m_type(Float)
  , m_componentCount(3)
{
  Allocate(3 * sizeof(float));
  SetFloatValue(v1, v2, v3);
}

UniformValue::UniformValue(const string & name, float v1, float v2, float v3, float v4)
  : m_name(name)
  , m_type(Float)
  , m_componentCount(4)
{
  Allocate(4 * sizeof(float));
  SetFloatValue(v1, v2, v3, v4);
}

UniformValue::UniformValue(const string & name, const float * matrixValue)
  : m_name(name)
  , m_type(Matrix4x4)
  , m_componentCount(16)
{
  Allocate(4 * 4 * sizeof(float));
  SetMatrix4x4Value(matrixValue);
}

const string & UniformValue::GetName() const
{
  return m_name;
}

UniformValue::Type UniformValue::GetType() const
{
  return m_type;
}

size_t UniformValue::GetComponentCount() const
{
  return m_componentCount;
}

void UniformValue::SetIntValue(int32_t v)
{
  ASSERT(m_type == Int, ());
  ASSERT(m_componentCount == 1, ());
  CopyValues(CastMemory<int32_t>(), &v, m_componentCount);
}

void UniformValue::SetIntValue(int32_t v1, int32_t v2)
{
  ASSERT(m_type == Int, ());
  ASSERT(m_componentCount == 2, ());
  int v[2] = { v1, v2 };
  CopyValues(CastMemory<int32_t>(), v, m_componentCount);
}

void UniformValue::SetIntValue(int32_t v1, int32_t v2, int32_t v3)
{
  ASSERT(m_type == Int, ());
  ASSERT(m_componentCount == 3, ());
  int v[3] = { v1, v2, v3 };
  CopyValues(CastMemory<int32_t>(), v, m_componentCount);
}

void UniformValue::SetIntValue(int32_t v1, int32_t v2, int32_t v3, int32_t v4)
{
  ASSERT(m_type == Int, ());
  ASSERT(m_componentCount == 4, ());
  int v[4] = { v1, v2, v3, v4 };
  CopyValues(CastMemory<int32_t>(), v, m_componentCount);
}

void UniformValue::SetFloatValue(float v)
{
  ASSERT(m_type == Float, ());
  ASSERT(m_componentCount == 1, ());
  CopyValues(CastMemory<float>(), &v, m_componentCount);
}

void UniformValue::SetFloatValue(float v1, float v2)
{
  ASSERT(m_type == Float, ());
  ASSERT(m_componentCount == 2, ());
  float v[2] = { v1, v2 };
  CopyValues(CastMemory<float>(), v, m_componentCount);
}

void UniformValue::SetFloatValue(float v1, float v2, float v3)
{
  ASSERT(m_type == Float, ());
  ASSERT(m_componentCount == 3, ());
  float v[3] = { v1, v2, v3 };
  CopyValues(CastMemory<float>(), v, m_componentCount);
}

void UniformValue::SetFloatValue(float v1, float v2, float v3, float v4)
{
  ASSERT(m_type == Float, ());
  ASSERT(m_componentCount == 4, ());
  float v[4] = { v1, v2, v3, v4 };
  CopyValues(CastMemory<float>(), v, m_componentCount);
}

void UniformValue::SetMatrix4x4Value(const float * matrixValue)
{
  ASSERT(m_type == Matrix4x4, ());
  ASSERT(m_componentCount == 16, ());
  memcpy(CastMemory<float>(), matrixValue, 4 * 4 * sizeof(float));
}

void UniformValue::Apply(RefPointer<GpuProgram> program) const
{
  ASSERT(program->HasUniform(m_name, GetCorrespondingGLType(), 1),
         ("Failed to find uniform", m_name, GetCorrespondingGLType(), 1));

  uint8_t location = program->GetUniformLocation(m_name);
  switch (m_type) {
  case Int:
    ApplyInt(location, CastMemory<int32_t>(), m_componentCount);
    break;
  case Float:
    ApplyFloat(location, CastMemory<float>(), m_componentCount);
    break;
  case Matrix4x4:
    ApplyMatrix(location, CastMemory<float>());
    break;
  default:
    ASSERT(false, ());
  }
}

#ifdef DEBUG
glConst UniformValue::GetCorrespondingGLType() const
{
  if (Int == m_type)
  {
    static glConst intTypes[4] = {
      GLConst::GLIntType,
      GLConst::GLIntVec2,
      GLConst::GLIntVec3,
      GLConst::GLIntVec4
    };
    return intTypes[m_componentCount - 1];
  }
  else if (Float == m_type)
  {
    static glConst floatTypes[4] = {
      GLConst::GLFloatType,
      GLConst::GLFloatVec2,
      GLConst::GLFloatVec3,
      GLConst::GLFloatVec4
    };
    return floatTypes[m_componentCount - 1];
  }
  else if (Matrix4x4 == m_type)
  {
    return GLConst::GLFloatMat4;
  }
  else
  {
    ASSERT(false, ());
    return -1;
  }
}
#endif

void UniformValue::Allocate(size_t byteCount)
{
  m_values.reset(new uint8_t[byteCount]);
}
