#include "drape/uniform_value.hpp"
#include "drape/gl_functions.hpp"

namespace dp
{
namespace
{
void ApplyInt(int8_t location, int32_t const * pointer, size_t componentCount)
{
  switch (componentCount)
  {
  case 1: GLFunctions::glUniformValuei(location, pointer[0]); break;
  case 2: GLFunctions::glUniformValuei(location, pointer[0], pointer[1]); break;
  case 3: GLFunctions::glUniformValuei(location, pointer[0], pointer[1], pointer[2]); break;
  case 4: GLFunctions::glUniformValuei(location, pointer[0], pointer[1], pointer[2], pointer[3]); break;
  default: ASSERT(false, ());
  }
}

void ApplyFloat(int8_t location, float const * pointer, size_t componentCount)
{
  switch (componentCount)
  {
  case 1: GLFunctions::glUniformValuef(location, pointer[0]); break;
  case 2: GLFunctions::glUniformValuef(location, pointer[0], pointer[1]); break;
  case 3: GLFunctions::glUniformValuef(location, pointer[0], pointer[1], pointer[2]); break;
  case 4: GLFunctions::glUniformValuef(location, pointer[0], pointer[1], pointer[2], pointer[3]); break;
  default: ASSERT(false, ());
  }
}

void ApplyMatrix(int8_t location, float const * matrix)
{
  GLFunctions::glUniformMatrix4x4Value(location, matrix);
}
}  // namespace

// static
void UniformValue::ApplyRaw(int8_t location, glsl::mat4 const & m)
{
  ASSERT_GREATER_OR_EQUAL(location, 0, ());
  ApplyMatrix(location, glsl::value_ptr(m));
}

// static
void UniformValue::ApplyRaw(int8_t location, float f)
{
  ApplyFloat(location, &f, 1);
}

// static
void UniformValue::ApplyRaw(int8_t location, glsl::vec2 const & v)
{
  ApplyFloat(location, glsl::value_ptr(v), 2);
}

// static
void UniformValue::ApplyRaw(int8_t location, glsl::vec3 const & v)
{
  ApplyFloat(location, glsl::value_ptr(v), 3);
}

// static
void UniformValue::ApplyRaw(int8_t location, glsl::vec4 const & v)
{
  ApplyFloat(location, glsl::value_ptr(v), 4);
}

// static
void UniformValue::ApplyRaw(int8_t location, int i)
{
  ApplyInt(location, &i, 1);
}

// static
void UniformValue::ApplyRaw(int8_t location, glsl::ivec2 const & v)
{
  ApplyInt(location, glsl::value_ptr(v), 2);
}

// static
void UniformValue::ApplyRaw(int8_t location, glsl::ivec3 const & v)
{
  ApplyInt(location, glsl::value_ptr(v), 3);
}

// static
void UniformValue::ApplyRaw(int8_t location, glsl::ivec4 const & v)
{
  ApplyInt(location, glsl::value_ptr(v), 4);
}
}  // namespace dp
