#pragma once

#include "../geometry/point2d.hpp"
#include "../../drape/color.hpp"

namespace glsl_types
{

struct vec2
{
  vec2() {}
  vec2(m2::PointF const & p)
    : x(p.x), y(p.y) {}

  vec2(float X, float Y = 0.0f)
    : x(X), y(Y) {}

  vec2 const & operator = (vec2 const & p);
  vec2 const & operator = (m2::PointF const & p);

  float x;
  float y;
};

struct vec3
{
  vec3() {}
  vec3(m2::PointF const & p, float Z)
    : x(p.x), y(p.y), z(Z) {}

  vec3(float X, m2::PointF const & p)
    : x(X), y(p.x), z(p.y) {}

  vec3(vec2 const & p, float Z)
    : x(p.x), y(p.y), z(Z) {}

  vec3(float X, vec2 const & p)
    : x(X), y(p.x), z(p.y) {}

  vec3(float X, float Y = 0.0f, float Z = 0.0f)
    : x(X), y(Y), z(Z) {}

  float x;
  float y;
  float z;
};

struct vec4
{
  vec4() {}
  vec4(m2::PointF const & p, m2::PointF const & t)
    : x(p.x), y(p.y), z(t.x), w(t.y) {}

  vec4(m2::PointF const & p, float Z, float W)
    : x(p.x), y(p.y), z(Z), w(W) {}

  vec4(float X, m2::PointF const & p, float W)
    : x(X), y(p.x), z(p.y), w(W) {}

  vec4(float X, float Y, m2::PointF const & p)
    : x(X), y(Y), z(p.x), w(p.y) {}

  vec4(vec2 const & p, vec2 const & t)
    : x(p.x), y(p.y), z(t.x), w(t.y) {}

  vec4(vec2 const & p, float Z, float W)
    : x(p.x), y(p.y), z(Z), w(W) {}

  vec4(float X, vec2 const & p, float W)
    : x(X), y(p.x), z(p.y), w(W) {}

  vec4(float X, float Y, vec2 const & p)
    : x(X), y(Y), z(p.x), w(p.y) {}

  vec4(float X, vec3 const & p)
    : x(X), y(p.x), z(p.y), w(p.z) {}

  vec4(vec3 const & p, float W)
    : x(p.x), y(p.y), z(p.z), w(W) {}

  vec4(float X, float Y = 0.0f, float Z = 0.0f, float W = 0.0f)
    : x(X), y(Y), z(Z), w(W) {}

  vec4(dp::ColorF const & clr)
    : x(clr.m_r), y(clr.m_g), z(clr.m_b), w(clr.m_a) {}

  float x;
  float y;
  float z;
  float w;
};

}
