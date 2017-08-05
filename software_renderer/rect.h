#ifndef __IA__RECT__
#define __IA__RECT__

#include "point.h"
#include <iostream>
#include <climits>

namespace ml
{
template <typename T>
struct rect_base
{
  typedef ml::point_base<T> point;

public:
  point min;
  point max;

  inline rect_base() : min(0, 0), max(0, 0) {}
  inline rect_base(double minx, double miny, double maxx, double maxy)
      : min(minx, miny), max(maxx, maxy)
  {
  }
  inline rect_base(const point & min_, const point & max_) : min(min_), max(max_) {}

  inline rect_base & set(const double & minx, const double & miny, const double & maxx,
                         const double & maxy)
  {
    min.x = minx;
    min.y = miny;
    max.x = maxx;
    max.y = maxy;
    return *this;
  }

  static rect_base void_rect()
  {
    return rect_base(std::numeric_limits<T>::max(), std::numeric_limits<T>::max(),
                     -std::numeric_limits<T>::max(), -std::numeric_limits<T>::max());
  }

  rect_base & set_left_top(const point & pt)
  {
    double sx = width();
    double sy = height();
    min = pt;
    max.x = min.x + sx;
    max.y = min.y + sy;
    return *this;
  }

  rect_base & set_base(const point & pt)
  {
    max.x = pt.x + max.x;
    min.x = pt.x + min.x;
    max.y = pt.y + max.y;
    min.y = pt.y + min.y;
    return *this;
  }

  inline bool contains(const point & pt) const { return pt.in_range(min, max); }

  int location(const rect_base & r) const
  {
    if ((r.max.x < min.x) || (r.min.x > max.x) || (r.max.y < min.y) || (r.min.y > max.y))
      return point::out;
    if ((r.min.x >= min.x) && (r.max.x <= max.x) && (r.min.y >= min.y) && (r.max.y <= max.y))
      return point::in;
    if ((r.min.x < min.x) && (r.max.x > max.x) && (r.min.y < min.y) && (r.max.y > max.y))
      return point::over;
    return point::cross;
  }

  rect_base & extend(const rect_base & r)
  {
    if (r.empty())
      return *this;
    min.x = std::min(min.x, std::min(r.min.x, r.max.x));
    min.y = std::min(min.y, std::min(r.min.y, r.max.y));

    max.x = std::max(max.x, std::max(r.min.x, r.max.x));
    max.y = std::max(max.y, std::max(r.min.y, r.max.y));
    return *this;
  }

  template <typename C>
  rect_base & bounds(C begin, C end)
  {
    for (; begin != end; ++begin)
    {
      min.x = std::min(min.x, begin->x);
      min.y = std::min(min.y, begin->y);
      max.x = std::max(max.x, begin->x);
      max.y = std::max(max.y, begin->y);
    }
    return *this;
  }

  rect_base & bounds(std::vector<point> const & v)
  {
    if (!v.empty())
    {
      min.x = max.x = v[0].x;
      min.y = max.y = v[0].y;
      for (size_t i = 1; i < v.size(); i++)
      {
        min.x = std::min(min.x, v[i].x);
        min.y = std::min(min.y, v[i].y);
        max.x = std::max(max.x, v[i].x);
        max.y = std::max(max.y, v[i].y);
      }
    }
    return *this;
  }

  rect_base & grow(double gs)
  {
    min.x -= gs;
    min.y -= gs;

    max.x += gs;
    max.y += gs;
    return *this;
  }

  rect_base & grow(const double dx, const double dy)
  {
    min.x -= dx;
    min.y -= dy;

    max.x += dx;
    max.y += dy;
    return *this;
  }

  inline double height() const { return fabs(max.y - min.y); }

  inline double width() const { return fabs(max.x - min.x); }

  inline point center() const { return point((max.x + min.x) / 2, (max.y + min.y) / 2); }

  bool operator==(rect_base const & r) const
  {
    return (r.min.x == min.x && r.min.y == min.y && r.max.x == max.x && r.max.y == max.y);
  }
  bool operator!=(rect_base const & r) const
  {
    return !(r.min.x == min.x && r.min.y == min.y && r.max.x == max.x && r.max.y == max.y);
  }

  bool empty() const
  {
    return (*this == rect_base::void_rect() || (width() == 0 && height() == 0));
  }
};

template <typename T>
inline std::ostream & operator<<(std::ostream & s, const rect_base<T> & r)
{
  return s << '[' << r.min << ',' << r.max << ']';
}

typedef rect_base<double> rect_d;
typedef rect_base<int> rect_i;

}  // namespace ml

#endif
