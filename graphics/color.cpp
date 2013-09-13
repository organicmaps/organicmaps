#include "color.hpp"


namespace graphics
{
  Color::Color(unsigned char r1, unsigned char g1, unsigned char b1, unsigned char a1)
    : r(r1), g(g1), b(b1), a(a1)
  {}

  Color::Color() : r(0), g(0), b(0), a(0)
  {}

  Color::Color(Color const & c)
  {
    r = c.r;
    g = c.g;
    b = c.b;
    a = c.a;
  }

  Color const & Color::operator=(Color const & c)
  {
    if (&c != this)
    {
      r = c.r;
      g = c.g;
      b = c.b;
      a = c.a;
    }
    return *this;
  }

  Color const Color::fromXRGB(uint32_t _c, unsigned char _a)
  {
    return Color(
        redFromARGB(_c),
        greenFromARGB(_c),
        blueFromARGB(_c),
        _a);
  }

  Color const Color::fromARGB(uint32_t _c)
  {
    return Color(
        redFromARGB(_c),
        greenFromARGB(_c),
        blueFromARGB(_c),
        alphaFromARGB(_c));
  }

  Color const & Color::operator /= (unsigned k)
  {
    r /= k;
    g /= k;
    b /= k;
    a /= k;

    return *this;
  }

  bool operator < (Color const & l, Color const & r)
  {
    if (l.r != r.r) return l.r < r.r;
    if (l.g != r.g) return l.g < r.g;
    if (l.b != r.b) return l.b < r.b;
    if (l.a != r.a) return l.a < r.a;
    return false;
  }

  bool operator > (Color const & l, Color const & r)
  {
    if (l.r != r.r) return l.r > r.r;
    if (l.g != r.g) return l.g > r.g;
    if (l.b != r.b) return l.b > r.b;
    if (l.a != r.a) return l.a > r.a;
    return false;
  }

  bool operator != (Color const & l, Color const & r)
  {
    return (l.r != r.r) || (l.g != r.g) || (l.b != r.b) || (l.a != r.a);
  }

  bool operator == (Color const & l, Color const & r)
  {
    return !(l != r);
  }

  Color Color::Black()
  {
    return Color(0, 0, 0, 255);
  }

  Color Color::White()
  {
    return Color(255, 255, 255, 255);
  }
}
