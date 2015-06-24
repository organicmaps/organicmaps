#pragma once

#include "std/cstdint.hpp"
#include "std/string.hpp"
#include "std/sstream.hpp"

namespace graphics
{

struct Color
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;

  /// alpha 255 means non-transparent and 0 means fully transparent
  Color(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t a1);
  Color();
  Color(Color const & p);
  Color const & operator=(Color const & p);

  static Color const fromARGB(uint32_t _c);
  static Color const fromXRGB(uint32_t _c, uint8_t _a = 0);

  Color const & operator /= (unsigned k);

  static Color Black();
  static Color White();
  static Color Red();
  static Color Green();
  static Color Blue();
};

bool operator < (Color const & l, Color const & r);
bool operator > (Color const & l, Color const & r);
bool operator !=(Color const & l, Color const & r);
bool operator ==(Color const & l, Color const & r);

inline int redFromARGB(uint32_t c)   {return (c >> 16) & 0xFF;}
inline int greenFromARGB(uint32_t c) {return (c >> 8) & 0xFF; }
inline int blueFromARGB(uint32_t c)  {return (c & 0xFF);      }
inline int alphaFromARGB(uint32_t c) {return (c >> 24) & 0xFF;}

inline string DebugPrint(Color const & c)
{
  ostringstream os;
  os << "r: " << (int)c.r << " ";
  os << "g: " << (int)c.g << " ";
  os << "b: " << (int)c.b << " ";
  os << "alpha: " << (int)c.a;
  return os.str();
}

}
