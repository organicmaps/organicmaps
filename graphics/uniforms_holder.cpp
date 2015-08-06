#include "graphics/uniforms_holder.hpp"

#include "std/utility.hpp"

namespace graphics
{

bool UniformsHolder::insertValue(ESemantic sem, float value)
{
  return insertValue(m_floatHolder, sem, value);
}

bool UniformsHolder::insertValue(ESemantic sem, float x, float y)
{
  return insertValue(m_vec2Holder, sem, m2::PointF(x, y));
}

bool UniformsHolder::insertValue(ESemantic sem, float x, float y, float z, float w)
{
  return insertValue(m_vec4Holder, sem, array<float, 4>({ x, y, z, w }));
}

bool UniformsHolder::insertValue(ESemantic sem, math::Matrix<float, 4, 4> const & matrix)
{
  return insertValue(m_mat4Holder, sem, matrix);
}

bool UniformsHolder::getValue(ESemantic sem, float & value) const
{
  return getValue(m_floatHolder, sem, value);
}

bool UniformsHolder::getValue(ESemantic sem, float & x, float & y) const
{
  m2::PointF pt;
  if (!getValue(m_vec2Holder, sem, pt))
    return false;

  x = pt.x;
  y = pt.y;
  return true;
}

bool UniformsHolder::getValue(ESemantic sem, float & x, float & y, float & z, float & w) const
{
  array<float, 4> v;
  if (!getValue(m_vec4Holder, sem, v))
    return false;

  x = v[0];
  y = v[1];
  z = v[2];
  w = v[3];
  return true;
}

bool UniformsHolder::getValue(ESemantic sem, math::Matrix<float, 4, 4> & value) const
{
  return getValue(m_mat4Holder, sem, value);
}

} // namespace graphics
