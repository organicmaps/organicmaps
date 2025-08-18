#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

namespace m2
{
namespace detail
{
// bit masks for code kinds
static int const LEFT = 1;
static int const RIGHT = 2;
static int const BOT = 4;
static int const TOP = 8;

// return accumulated bit mask for point-rect interaction
template <class T>
int vcode(m2::Rect<T> const & r, m2::Point<T> const & p)
{
  return ((p.x < r.minX() ? LEFT : 0) + (p.x > r.maxX() ? RIGHT : 0) + (p.y < r.minY() ? BOT : 0) +
          (p.y > r.maxY() ? TOP : 0));
}
}  // namespace detail

template <class T>
bool Intersect(m2::Rect<T> const & r, m2::Point<T> & p1, m2::Point<T> & p2, int & code1, int & code2)
{
  code1 = code2 = 0;
  int codeClip[2] = {0, 0};
  int code[2] = {detail::vcode(r, p1), detail::vcode(r, p2)};

  // do while one of the point is out of rect
  while (code[0] || code[1])
  {
    if (code[0] & code[1])
    {
      // both point area on the one side of rect
      return false;
    }

    // choose point with non-zero code
    m2::Point<T> * pp;
    int i;
    if (code[0])
    {
      i = 0;
      pp = &p1;
    }
    else
    {
      i = 1;
      pp = &p2;
    }

    // added points compare to avoid NAN numbers
    if (code[i] & detail::LEFT)
    {
      if (p1 == p2)
        return false;
      pp->y += (p1.y - p2.y) * (r.minX() - pp->x) / (p1.x - p2.x);
      pp->x = r.minX();
      codeClip[i] = detail::LEFT;
    }
    else if (code[i] & detail::RIGHT)
    {
      if (p1 == p2)
        return false;
      pp->y += (p1.y - p2.y) * (r.maxX() - pp->x) / (p1.x - p2.x);
      pp->x = r.maxX();
      codeClip[i] = detail::RIGHT;
    }

    if (code[i] & detail::BOT)
    {
      if (p1 == p2)
        return false;
      pp->x += (p1.x - p2.x) * (r.minY() - pp->y) / (p1.y - p2.y);
      pp->y = r.minY();
      codeClip[i] = detail::BOT;
    }
    else if (code[i] & detail::TOP)
    {
      if (p1 == p2)
        return false;
      pp->x += (p1.x - p2.x) * (r.maxY() - pp->y) / (p1.y - p2.y);
      pp->y = r.maxY();
      codeClip[i] = detail::TOP;
    }

    // update code with new point
    code[i] = detail::vcode(r, *pp);
  }

  code1 = codeClip[0];
  code2 = codeClip[1];
  // both codes are equal to zero => points area inside rect
  return true;
}

template <class T>
bool Intersect(m2::Rect<T> const & r, m2::Point<T> & p1, m2::Point<T> & p2)
{
  int code1, code2;
  return Intersect(r, p1, p2, code1, code2);
}
}  // namespace m2
