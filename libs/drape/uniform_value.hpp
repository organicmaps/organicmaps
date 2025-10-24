#pragma once

#include "drape/glsl_types.hpp"

#include <cstdint>

namespace dp
{
class UniformValue
{
public:
  static void ApplyRaw(int location, glsl::mat4 const & m);
  static void ApplyRaw(int location, float f);
  static void ApplyRaw(int location, glsl::vec2 const & v);
  static void ApplyRaw(int location, glsl::vec3 const & v);
  static void ApplyRaw(int location, glsl::vec4 const & v);
  static void ApplyRaw(int location, glsl::vec4 const * v, uint32_t count);
  static void ApplyRaw(int location, int i);
  static void ApplyRaw(int location, int const * i, uint32_t count);
  static void ApplyRaw(int location, glsl::ivec2 const & v);
  static void ApplyRaw(int location, glsl::ivec3 const & v);
  static void ApplyRaw(int location, glsl::ivec4 const & v);
};
}  // namespace dp
