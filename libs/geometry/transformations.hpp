#pragma once

#include "geometry/point2d.hpp"

#include "base/matrix.hpp"

/// v` = v * M, v = [x, y, 1]

namespace math
{
template <typename T, typename U>
Matrix<T, 3, 3> const Rotate(Matrix<T, 3, 3> const & m, U const & cos, U const & sin)
{
  Matrix<T, 3, 3> m1 = Identity<T, 3>();
  m1(0, 0) = cos;
  m1(0, 1) = sin;
  m1(1, 0) = -sin;
  m1(1, 1) = cos;
  return m * m1;
}

template <typename T, typename U>
Matrix<T, 3, 3> const Rotate(Matrix<T, 3, 3> const & m, U const & angle)
{
  return Rotate(m, cos(angle), sin(angle));
}

template <typename T, typename U>
Matrix<T, 3, 3> const Shift(Matrix<T, 3, 3> const & m, U const & dx, U const & dy)
{
  Matrix<T, 3, 3> m1 = Identity<T, 3>();
  m1(2, 0) = dx;
  m1(2, 1) = dy;
  return m * m1;
}

template <typename T, typename U>
Matrix<T, 3, 3> const Shift(Matrix<T, 3, 3> const & m, m2::Point<U> const & pt)
{
  return Shift(m, pt.x, pt.y);
}

template <typename T, typename U>
Matrix<T, 3, 3> const Scale(Matrix<T, 3, 3> const & m, U const & kx, U const & ky)
{
  Matrix<T, 3, 3> m1 = Identity<T, 3>();
  m1(0, 0) = kx;
  m1(1, 1) = ky;
  return m * m1;
}

template <typename T, typename U>
Matrix<T, 3, 3> const Scale(Matrix<T, 3, 3> const & m, m2::Point<U> const & pt)
{
  return Scale(m, pt.x, pt.y);
}
}  // namespace math
