#pragma once

#include "drape/pointers.hpp"
#include "drape/gpu_program.hpp"

#include "std/string.hpp"
#include "std/shared_array.hpp"
#include "std/cstring.hpp"

namespace dp
{

class UniformValue
{
public:
  enum Type
  {
    Int,
    Float,
    Matrix4x4
  };

  explicit UniformValue(string const & name, int32_t v);
  explicit UniformValue(string const & name, int32_t v1, int32_t v2);
  explicit UniformValue(string const & name, int32_t v1, int32_t v2, int32_t v3);
  explicit UniformValue(string const & name, int32_t v1, int32_t v2, int32_t v3, int32_t v4);

  explicit UniformValue(string const & name, float v);
  explicit UniformValue(string const & name, float v1, float v2);
  explicit UniformValue(string const & name, float v1, float v2, float v3);
  explicit UniformValue(string const & name, float v1, float v2, float v3, float v4);

  explicit UniformValue(string const & name, float const * matrixValue);

  string const & GetName() const;
  Type GetType() const;
  size_t GetComponentCount() const;

  void SetIntValue(int32_t v);
  void SetIntValue(int32_t v1, int32_t v2);
  void SetIntValue(int32_t v1, int32_t v2, int32_t v3);
  void SetIntValue(int32_t v1, int32_t v2, int32_t v3, int32_t v4);

  void SetFloatValue(float v);
  void SetFloatValue(float v1, float v2);
  void SetFloatValue(float v1, float v2, float v3);
  void SetFloatValue(float v1, float v2, float v3, float v4);

  void SetMatrix4x4Value(float const * matrixValue);

  void Apply(ref_ptr<GpuProgram> program) const;

  bool operator<(UniformValue const & other) const
  {
    if (m_name != other.m_name)
      return m_name < other.m_name;
    if (m_type != other.m_type)
      return m_type < other.m_type;
    if (m_componentCount != other.m_componentCount)
      return m_componentCount < other.m_componentCount;

    size_t s = ((m_type == Int) ? sizeof(int32_t) : sizeof(float)) * m_componentCount;
    return memcmp(m_values.get(), other.m_values.get(), s) < 0;
  }

private:
  void Allocate(size_t byteCount);

  template <typename T>
  T * CastMemory()
  {
    return reinterpret_cast<T *>(m_values.get());
  }

  template <typename T>
  const T * CastMemory() const
  {
    return reinterpret_cast<T *>(m_values.get());
  }

private:
  string m_name;
  Type m_type;
  size_t m_componentCount;

  shared_array<uint8_t> m_values;
};

} // namespace dp
