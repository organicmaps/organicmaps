#pragma once

#include "../std/stdint.hpp"

struct Color
{
  Color();
  Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alfa);

  uint8_t m_red;
  uint8_t m_green;
  uint8_t m_blue;
  uint8_t m_alfa;

  bool operator <  (Color const & other) const;
  bool operator == (Color const & other) const;
};

struct ColorF
{
  ColorF() {}
  ColorF(Color const & clr)
          : m_r(clr.m_red / 255.0f), m_g(clr.m_green / 255.0f),
            m_b(clr.m_blue / 255.0f), m_a(clr.m_alfa / 255.0f) {}
  ColorF(float r, float g, float b, float a) : m_r (r), m_g (g), m_b (b), m_a (a) {}

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


