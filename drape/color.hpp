#pragma once

#include "std/cstdint.hpp"
#include "std/sstream.hpp"
#include "std/string.hpp"

namespace dp
{

struct Color
{
  Color();
  Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alfa);

  uint8_t GetRed() const;
  uint8_t GetGreen() const;
  uint8_t GetBlue() const;
  uint8_t GetAlfa() const;

  bool operator==(Color const & other) const { return m_rgba == other.m_rgba; }
  bool operator< (Color const & other) const { return m_rgba < other.m_rgba; }

  static Color Black()             { return Color(0, 0, 0, 255); }
  static Color White()             { return Color(255, 255, 255, 255); }
  static Color Red()               { return Color(255, 0, 0, 255); }
  static Color Transparent()       { return Color(0, 0, 0, 0); }
  static Color RoadNumberOutline() { return Color(150, 75, 0, 255); }

private:
  uint32_t m_rgba;
};

inline uint8_t ExtractRed(uint32_t argb);
inline uint8_t ExtractGreen(uint32_t argb);
inline uint8_t ExtractBlue(uint32_t argb);
inline uint8_t ExtractAlfa(uint32_t argb);
Color Extract(uint32_t argb);
Color Extract(uint32_t xrgb, uint8_t a);

inline string DebugPrint(Color const & c)
{
  ostringstream out;
  out << "R = " << c.GetRed()
      << "G = " << c.GetGreen()
      << "B = " << c.GetBlue()
      << "A = " << c.GetAlfa();
  return out.str();
}

}
