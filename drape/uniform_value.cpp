#include "uniform_value.hpp"
#include "glfunctions.hpp"
#include "../base/assert.hpp"

namespace
{
  template<typename T>
  void CopyValues(T * dstPointer, T * values, size_t valuesCount)
  {
    for (size_t i = 0; i < valuesCount; ++i)
      dstPointer[i] = values[i];
  }

  template<typename T>
  void ApplyScalar(int8_t location, T * pointer, size_t componentCount)
  {
    switch (componentCount)
    {
    case 1:
      GLFunctions::glUniformValue(location, pointer[0]);
      break;
    case 2:
      GLFunctions::glUniformValue(location, pointer[0], pointer[1]);
      break;
    case 3:
      GLFunctions::glUniformValue(location, pointer[0], pointer[1], pointer[2]);
      break;
    case 4:
      GLFunctions::glUniformValue(location, pointer[0], pointer[1], pointer[2], pointer[3]);
      break;
    default:
      ASSERT(false, ());
    }
  }

  void ApplyMatrix(uint8_t location, float * matrix)
  {
    GLFunctions::glUniformMatrix4x4Value(location, matrix);
  }
}

UniformValue::UniformValue(const string & name, int32_t v)
  : m_name(name)
{
  Allocate(sizeof(int32_t));
  CopyValues(CastMemory<int32_t>(), &v, 1);
  m_type = Int;
  m_componentCount = 1;
}

UniformValue::UniformValue(const string & name, int32_t v1, int32_t v2)
  : m_name(name)
{
  Allocate(2 * sizeof(int32_t));

  int32_t values[2] = { v1, v2 };
  CopyValues(CastMemory<int32_t>(), values, 2);
  m_type = Int;
  m_componentCount = 2;
}

UniformValue::UniformValue(const string & name, int32_t v1, int32_t v2, int32_t v3)
  : m_name(name)
{
  Allocate(3 * sizeof(int32_t));

  int32_t values[3] = { v1, v2, v3 };
  CopyValues(CastMemory<int32_t>(), values, 3);
  m_type = Int;
  m_componentCount = 3;
}

UniformValue::UniformValue(const string & name, int32_t v1, int32_t v2, int32_t v3, int32_t v4)
  : m_name(name)
{
  Allocate(4 * sizeof(int32_t));
  int32_t values[4] = { v1, v2, v3, v4 };
  CopyValues(CastMemory<int32_t>(), values, 4);
  m_type = Int;
  m_componentCount = 4;
}

UniformValue::UniformValue(const string & name, float v)
  : m_name(name)
{
  Allocate(sizeof(float));
  CopyValues(CastMemory<float>(), &v, 1);
  m_type = Float;
  m_componentCount = 1;
}

UniformValue::UniformValue(const string & name, float v1, float v2)
  : m_name(name)
{
  Allocate(2 * sizeof(float));
  float values[2] = { v1, v2 };
  CopyValues(CastMemory<float>(), values, 2);
  m_type = Float;
  m_componentCount = 2;
}

UniformValue::UniformValue(const string & name, float v1, float v2, float v3)
  : m_name(name)
{
  Allocate(3 * sizeof(float));
  float values[3] = { v1, v2, v3 };
  CopyValues(CastMemory<float>(), values, 3);
  m_type = Float;
  m_componentCount = 3;
}

UniformValue::UniformValue(const string & name, float v1, float v2, float v3, float v4)
  : m_name(name)
{
  Allocate(4 * sizeof(float));
  float values[4] = { v1, v2, v3, v4 };
  CopyValues(CastMemory<float>(), values, 4);
  m_type = Float;
  m_componentCount = 4;
}

UniformValue::UniformValue(const string & name, float * matrixValue)
  : m_name(name)
{
  Allocate(4 * 4 * sizeof(float));
  memcpy(CastMemory<float>(), matrixValue, 4 * 4 * sizeof(float));
  m_type = Matrix4x4;
  m_componentCount = 16;
}

void UniformValue::Apply(WeakPointer<GpuProgram> program)
{
  uint8_t location = program->GetUniformLocation(m_name);
  switch (m_type) {
  case Int:
    ApplyScalar(location, CastMemory<int32_t>(), m_componentCount);
    break;
  case Float:
    ApplyScalar(location, CastMemory<float>(), m_componentCount);
    break;
  case Matrix4x4:
    ApplyMatrix(location, CastMemory<float>());
    break;
  default:
    ASSERT(false, ());
  }
}

void UniformValue::Allocate(size_t byteCount)
{
  m_values = make_shared_ptr(new uint8_t[byteCount]);
}
