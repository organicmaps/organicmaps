#include "common_structures.hpp"

namespace glsl_types
{

vec2 const & vec2::operator = (vec2 const & p)
{
  x = p.x;
  y = p.y;
  return *this;
}

vec2 const & vec2::operator = (m2::PointF const & p)
{
  x = p.x;
  y = p.y;
  return *this;
}

}

