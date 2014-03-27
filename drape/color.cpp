#include "color.hpp"

namespace
{
  static const uint8_t MaxChannelValue = 255;
}

Color::Color()
  : m_red(0)
  , m_green(0)
  , m_blue(0)
  , m_alfa(MaxChannelValue)
{
}

Color::Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alfa)
  : m_red(red)
  , m_green(green)
  , m_blue(blue)
  , m_alfa(alfa)
{
}

bool Color::operator< (Color const & other) const
{
  if (other.m_red != m_red)
    return m_red < other.m_red;
  if (other.m_green != m_green)
    return m_green < other.m_green;
  if (other.m_blue != m_blue)
    return m_blue < other.m_blue;

  return m_alfa < other.m_alfa;
}

bool Color::operator== (Color const & other) const
{
  return m_red   == other.m_red   &&
         m_green == other.m_green &&
         m_blue  == other.m_blue  &&
         m_alfa  == other.m_alfa;
}

uint8_t ExtractRed(uint32_t argb)
{
  return (argb >> 16) & 0xFF;
}

uint8_t ExtractGreen(uint32_t argb)
{
  return (argb >> 8) & 0xFF;
}

uint8_t ExtractBlue(uint32_t argb)
{
  return argb & 0xFF;
}

uint8_t ExtractAlfa(uint32_t argb)
{
  return (argb >> 24) & 0xFF;
}

Color Extract(uint32_t argb)
{
  return Color(ExtractRed(argb),
               ExtractGreen(argb),
               ExtractBlue(argb),
               ExtractAlfa(argb));
}

Color Extract(uint32_t xrgb, uint8_t a)
{
  return Color(ExtractRed(xrgb),
               ExtractGreen(xrgb),
               ExtractBlue(xrgb),
               a);
}

void Convert(Color const & c, float & r, float & g, float & b, float & a)
{
  r = c.m_red / (float)MaxChannelValue;
  g = c.m_green / (float)MaxChannelValue;
  b = c.m_blue / (float)MaxChannelValue;
  a = c.m_alfa / (float)MaxChannelValue;
}
