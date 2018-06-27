#pragma once

#include "drape/glsl_types.hpp"
#include "drape/gpu_program.hpp"
#include "drape/pointers.hpp"

#include <boost/shared_array.hpp>

#include <cstring>
#include <string>

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

  explicit UniformValue(std::string const & name, int32_t v);
  explicit UniformValue(std::string const & name, int32_t v1, int32_t v2);
  explicit UniformValue(std::string const & name, int32_t v1, int32_t v2, int32_t v3);
  explicit UniformValue(std::string const & name, int32_t v1, int32_t v2, int32_t v3, int32_t v4);

  explicit UniformValue(std::string const & name, float v);
  explicit UniformValue(std::string const & name, float v1, float v2);
  explicit UniformValue(std::string const & name, float v1, float v2, float v3);
  explicit UniformValue(std::string const & name, float v1, float v2, float v3, float v4);

  explicit UniformValue(std::string const & name, float const * matrixValue);

  std::string const & GetName() const;
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

  static void ApplyRaw(int8_t location, glsl::mat4 const & m);
  static void ApplyRaw(int8_t location, float f);
  static void ApplyRaw(int8_t location, glsl::vec2 const & v);
  static void ApplyRaw(int8_t location, glsl::vec3 const & v);
  static void ApplyRaw(int8_t location, glsl::vec4 const & v);
  static void ApplyRaw(int8_t location, int i);
  static void ApplyRaw(int8_t location, glsl::ivec2 const & v);
  static void ApplyRaw(int8_t location, glsl::ivec3 const & v);
  static void ApplyRaw(int8_t location, glsl::ivec4 const & v);

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
  std::string m_name;
  Type m_type;
  size_t m_componentCount;

  boost::shared_array<uint8_t> m_values;
};
}  // namespace dp
