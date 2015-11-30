#ifndef __IA__POINT__
#define __IA__POINT__

#include <cmath>
#include <iomanip>
#include <limits>

namespace ml
{
template <typename T>
struct point_base
{
  enum relation
  {
    left,
    right,
    beyond,
    behind,
    between,
    origin,
    destination
  };
  enum placement
  {
    out,
    in,
    cross,
    over,
    border,
    paralel,
    none
  };

  T x;
  T y;

  inline point_base()
      : x(std::numeric_limits<T>::quiet_NaN()), y(std::numeric_limits<T>::quiet_NaN())
  {
  }
  inline point_base(T ix, T iy) : x(ix), y(iy) {}

  inline operator bool() const { return (!std::isnan(x) && !std::isnan(y)); }

  T lon() const { return x; }
  T lon(T l) { return (x = l); }

  T lat() const { return y; }
  T lat(T l) { return (y = l); }

  bool operator==(point_base const & p) const { return (x == p.x) && (y == p.y); }
  bool operator!=(point_base const & p) const { return (x != p.x) || (y != p.y); }

  inline bool in_range(point_base const & a, point_base const & b) const
  {
    return (std::min(a.x, b.x) <= x && x <= std::max(a.x, b.x)) &&
           (std::min(a.y, b.y) <= y && y <= std::max(a.y, b.y));
  }

  bool operator<(point_base const & p) const { return (y == p.y) ? (x < p.x) : (y < p.y); }

  double length() const { return sqrt(x * x + y * y); }

  double length2() const { return x * x + y * y; }

  double angle(point_base const & pt) const  // return angle in radians
  {
    double dx = pt.x - x;
    double dy = pt.y - y;
    return ((dx == 0) ? (3.1415 / 2 * (dy > 0 ? 1 : -1)) : atan(dy / dx)) - (dx < 0 ? 3.1415 : 0);
  }

  double norm_angle(point_base const & pt) const  // return angle in radians
  {
    double dx = pt.x - x;
    double dy = pt.y - y;
    return atan(-dy / dx) + ((dx < 0) ? 3.1415926536 : (dy < 0 ? 0 : 3.1415926536 * 2));
  }

  point_base<T> vector(double length, double angle) const
  {
    return point_base<T>(x + length * cos(angle), y + length * sin(angle));
  }

  int classify(point_base const & p0, point_base const & p1) const
  {
    point_base a(p1.x - p0.x, p1.y - p0.y);
    point_base b(x - p0.x, y - p0.y);
    double sa = a.x * b.y - b.x * a.y;

    if (sa > 0.0)
      return left;
    if (sa < 0.0)
      return right;
    if ((a.x * b.x < 0.0) || (a.y * b.y < 0.0))
      return behind;
    if ((a.x * a.x + a.y * a.y) < (b.x * b.x + b.y * b.y))
      return beyond;
    if (p0 == *this)
      return origin;
    if (p1 == *this)
      return destination;

    return between;
  }
};

template <typename T>
inline point_base<T> operator*(point_base<T> const & p, T c)
{
  return point_base<T>(p.x * c, p.y * c);
}

template <typename T>
inline T operator*(point_base<T> const & p1, point_base<T> const & p2)
{
  return p1.x * p2.x + p1.y * p2.y;
}

template <typename T>
inline point_base<T> operator+(point_base<T> const & p1, point_base<T> const & p2)
{
  return point_base<T>(p2.x + p1.x, p2.y + p1.y);
}

template <typename T>
inline point_base<T> operator-(point_base<T> const & p1, point_base<T> const & p2)
{
  return point_base<T>(p1.x - p2.x, p1.y - p2.y);
}

template <typename T>
inline double distance(point_base<T> const & p1, point_base<T> const & p2)
{
  double dx = (p1.x - p2.x);
  double dy = (p1.y - p2.y);
  return sqrt(dx * dx + dy * dy);
}

template <typename T>
inline double dot(point_base<T> const & p1, point_base<T> const & p2)
{
  return p1.x * p2.x + p1.y * p2.y;
}

template <typename T>
point_base<T> point_on_segment(point_base<T> const & p1, point_base<T> const & p2,
                               point_base<T> const & pt)
{
  point_base<T> v21 = p2 - p1;
  point_base<T> v01 = pt - p1;

  double e = v21 * v01 / v21.length2();

  if ((v01 * v21 >= 0.0) && ((v01 * v21) / (v21.length2()) <= 1.0))
    return p1 + v21 * e;
  else
    return ml::point_base<T>();
}

template <typename T>
inline double distance_to_segment(point_base<T> const & p1, point_base<T> const & p2,
                                  point_base<T> const & p)
{
  double dx = (p2.x - p1.x);
  double dy = (p2.y - p1.y);
  return (dx * (p.y - p1.y) - dy * (p.x - p1.x)) / sqrt(dx * dx + dy * dy);
}

// slow nearest point implementation
// returns intersection with normal point in p variable
template <typename T>
inline point_base<T> nearest_point(point_base<T> const & p1, point_base<T> const & p2,
                                   point_base<T> const & p)
{
  point_base<T> v = p2 - p1;
  point_base<T> w = p - p1;

  double c1 = dot(w, v);
  if (c1 <= 0)
    return p1;

  double c2 = dot(v, v);
  if (c2 <= c1)
    return p2;

  double b = c1 / c2;
  point_base<T> Pb = p1 + point_base<T>(b * v.x, b * v.y);
  return Pb;
}

template <typename T>
inline bool is_intersect(double x11, double y11, double x12, double y12, double x21, double y21,
                         double x22, double y22, point_base<T> * pt = NULL)
{
  double v = ((x12 - x11) * (y21 - y11) + (y12 - y11) * (x11 - x21)) /
             ((y12 - y11) * (x22 - x21) - (x12 - x11) * (y22 - y21));
  point_base<T> p(x21 + (x22 - x21) * v, y21 + (y22 - y21) * v);

  if (!(((p.x >= x11) && (p.x <= x12)) || ((p.x <= x11) && (p.x >= x12))))
    return false;
  if (!(((p.x >= x21) && (p.x <= x22)) || ((p.x <= x21) && (p.x >= x22))))
    return false;
  if (!(((p.y >= y11) && (p.y <= y12)) || ((p.y <= y11) && (p.y >= y12))))
    return false;
  if (!(((p.y >= y21) && (p.y <= y22)) || ((p.y <= y21) && (p.y >= y22))))
    return false;

  if (pt)
  {
    pt->x = p.x;
    pt->y = p.y;
  }
  return true;
}

template <typename T>
inline bool is_intersect(point_base<T> const & p1, point_base<T> const & p2,
                         point_base<T> const & p3, point_base<T> const & p4,
                         point_base<T> * pt = NULL)
{
  return is_intersect(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, pt);
}

template <typename Container>
inline double length(Container const & v)
{
  double len = 0.0;
  for (size_t i = 1; i < v.size(); ++i)
    len += distance(v[i - 1], v[i]);
  return len;
}

template <typename T>
inline int segment(std::vector<point_base<T> > const & v, double length, double * out_length = NULL)
{
  if (v.size() < 2)
    return -1;

  int segment_num = 0;
  double segment_length = ml::distance(v[segment_num], v[segment_num + 1]);
  while ((length - segment_length >= 0) && (segment_num < v.size() - 1))
  {
    length -= segment_length;
    segment_num++;
    segment_length = ml::distance(v[segment_num], v[segment_num + 1]);
  }
  if (out_length)
    *out_length = length;
  return segment_num;
}

template <typename T>
inline std::ostream & operator<<(std::ostream & s, point_base<T> const & p)
{
  return s << p.x << ", " << p.y;
}

typedef point_base<double> point_d;
typedef point_base<int> point_i;
}

#endif
