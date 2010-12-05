#pragma once

#include "rect2d.hpp"
#include "point2d.hpp"

namespace m2
{
  namespace detail
  {
    static const int LEFT = 1;    // двоичное 0001
    static const int RIGHT = 2;   // двоичное 0010
    static const int BOT = 4;     // двоичное 0100
    static const int TOP = 8;     // двоичное 1000

    template <class T>
    int vcode(m2::Rect<T> const & r, m2::Point<T> const & p)
    {
      return ((p.x < r.minX() ? LEFT : 0)  +    // +1 если точка левее прямоугольника
              (p.x > r.maxX() ? RIGHT : 0) +    // +2 если точка правее прямоугольника
              (p.y < r.minY() ? BOT : 0)   +    // +4 если точка ниже прямоугольника
              (p.y > r.maxY() ? TOP : 0));      // +8 если точка выше прямоугольника
    }
  }

  template <class T>
  bool Intersect(m2::Rect<T> const & r, m2::Point<T> & p1, m2::Point<T> & p2)
  {
    int code1 = detail::vcode(r, p1);
    int code2 = detail::vcode(r, p2);

    // пока одна из точек отрезка вне прямоугольника
    while (code1 || code2)
    {
      if (code1 & code2)
      {
        // если обе точки с одной стороны прямоугольника, то отрезок не пересекает прямоугольник
        return false;
      }

      // выбираем точку c с ненулевым кодом
      m2::Point<T> * pp;
      int code;
      if (code1)
      {
        code = code1;
        pp = &p1;
      }
      else
      {
        code = code2;
        pp = &p2;
      }

      // если pp левее r, то передвигаем pp на прямую x = r->x_min
      // если pp правее r, то передвигаем pp на прямую x = r->x_max
      if (code & detail::LEFT)
      {
        pp->y += (p1.y - p2.y) * (r.minX() - pp->x) / (p1.x - p2.x);
        pp->x = r.minX();
      }
      else if (code & detail::RIGHT)
      {
        pp->y += (p1.y - p2.y) * (r.maxX() - pp->x) / (p1.x - p2.x);
        pp->x = r.maxX();
      }

      // если pp ниже r, то передвигаем pp на прямую y = r->y_min
      // если pp выше r, то передвигаем pp на прямую y = r->y_max
      if (code & detail::BOT)
      {
        pp->x += (p1.x - p2.x) * (r.minY() - pp->y) / (p1.y - p2.y);
        pp->y = r.minY();
      }
      else if (code & detail::TOP)
      {
        pp->x += (p1.x - p2.x) * (r.maxY() - pp->y) / (p1.y - p2.y);
        pp->y = r.maxY();
      }

      // обновляем код
      if (code == code1)
        code1 = detail::vcode(r, p1);
      else
        code2 = detail::vcode(r, p2);
    }

    // оба кода равны 0, следовательно обе точки в прямоугольнике
    return true;
  }
}
