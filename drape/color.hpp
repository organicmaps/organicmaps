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

inline uint8_t ExtractRed(uint32_t argb);
inline uint8_t ExtractGreen(uint32_t argb);
inline uint8_t ExtractBlue(uint32_t argb);
inline uint8_t ExtractAlfa(uint32_t argb);
Color Extract(uint32_t argb);
Color Extract(uint32_t xrgb, uint8_t a);
void  Convert(Color const & c, float & r, float & g, float & b, float & a);


