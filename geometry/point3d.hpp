#pragma once

#include <cmath>
#include <sstream>
#include <string>

namespace m3
{
template <typename T>
class Point
{
public:
  constexpr Point() : x(T()), y(T()), z(T()) {}
  constexpr Point(T const & x, T const & y, T const & z) : x(x), y(y), z(z) {}

  T Length() const { return std::sqrt(x * x + y * y + z * z); }

  bool operator==(Point<T> const & rhs) const;

  T x;
  T y;
  T z;
};

template <typename T>
bool Point<T>::operator==(m3::Point<T> const & rhs) const
{
  return x == rhs.x && y == rhs.y && z == rhs.z;
}

template <typename T>
T DotProduct(Point<T> const & a, Point<T> const & b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <typename T>
Point<T> CrossProduct(Point<T> const & a, Point<T> const & b)
{
  auto const x = a.y * b.z - a.z * b.y;
  auto const y = a.z * b.x - a.x * b.z;
  auto const z = a.x * b.y - a.y * b.x;
  return Point<T>(x, y, z);
}

using PointD = Point<double>;

template <typename T>
std::string DebugPrint(Point<T> const & p)
{
  std::ostringstream out;
  out.precision(20);
  out << "m3::Point<" << typeid(T).name() << ">(" << p.x << ", " << p.y << ", " << p.z << ")";
  return out.str();
}
}  // namespace m3
