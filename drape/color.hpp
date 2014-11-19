#pragma once

#include "../std/stdint.hpp"

namespace dp
{

struct Color
{
  Color();
  Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alfa);

  bool operator <  (Color const & other) const;
  bool operator == (Color const & other) const;

  uint8_t m_red;
  uint8_t m_green;
  uint8_t m_blue;
  uint8_t m_alfa;

  int GetColorInInt() const { return (m_alfa << 24) | (m_blue << 16) | (m_green << 8) | m_red; }

  static Color Black() { return Color(0, 0, 0, 255); }
  static Color White() { return Color(255, 255, 255, 255); }
  static Color Red()   { return Color(255, 0, 0, 255); }
};

struct ColorF
{
  ColorF() {}
  ColorF(Color const & clr);
  ColorF(float r, float g, float b, float a) : m_r (r), m_g (g), m_b (b), m_a (a) {}

  bool operator <  (ColorF const & other) const;
  bool operator == (ColorF const & other) const;

  float m_r;
  float m_g;
  float m_b;
  float m_a;
};

inline uint8_t ExtractRed(uint32_t argb);
inline uint8_t ExtractGreen(uint32_t argb);
inline uint8_t ExtractBlue(uint32_t argb);
inline uint8_t ExtractAlfa(uint32_t argb);
Color Extract(uint32_t argb);
Color Extract(uint32_t xrgb, uint8_t a);
void  Convert(Color const & c, float & r, float & g, float & b, float & a);

}
