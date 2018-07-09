#pragma once

#include "drape/glsl_types.hpp"

#include <cstdint>

namespace dp
{
class UniformValue
{
public:
  static void ApplyRaw(int8_t location, glsl::mat4 const & m);
  static void ApplyRaw(int8_t location, float f);
  static void ApplyRaw(int8_t location, glsl::vec2 const & v);
  static void ApplyRaw(int8_t location, glsl::vec3 const & v);
  static void ApplyRaw(int8_t location, glsl::vec4 const & v);
  static void ApplyRaw(int8_t location, int i);
  static void ApplyRaw(int8_t location, glsl::ivec2 const & v);
  static void ApplyRaw(int8_t location, glsl::ivec3 const & v);
  static void ApplyRaw(int8_t location, glsl::ivec4 const & v);
};
}  // namespace dp
