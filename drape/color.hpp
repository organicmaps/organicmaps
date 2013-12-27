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

  bool operator <  (Color const & other);
  bool operator == (Color const & other);
};

inline uint8_t ExtractRed(uint32_t argb);
inline uint8_t ExtractGreen(uint32_t argb);
inline uint8_t ExtractBlue(uint32_t argb);
inline uint8_t ExtractAlfa(uint32_t argb);
inline Color Extract(uint32_t argb);
inline void Convert(Color const & c, float & r, float & g, float & b, float & a);


