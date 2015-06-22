#include "drape/color.hpp"

namespace dp
{

namespace
{

static const uint8_t MaxChannelValue = 255;

} // namespace

#define EXTRACT_BYTE(x, n) (((x) >> 8 * (n)) & 0xFF);
#define CHANNEL_TO_FLOAT(x) ((x) / static_cast<float>(MaxChannelValue))

Color::Color()
  : m_rgba(MaxChannelValue)
{
}

Color::Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alfa)
{
  m_rgba = red << 24 | green << 16 | blue << 8 | alfa;
}

uint8_t Color::GetRed() const
{
  return EXTRACT_BYTE(m_rgba, 3);
}

uint8_t Color::GetGreen() const
{
  return EXTRACT_BYTE(m_rgba, 2);
}

uint8_t Color::GetBlue() const
{
  return EXTRACT_BYTE(m_rgba, 1);
}

uint8_t Color::GetAlfa() const
{
  return EXTRACT_BYTE(m_rgba, 0);
}

float Color::GetRedF() const
{
  return CHANNEL_TO_FLOAT(GetRed());
}

float Color::GetGreenF() const
{
  return CHANNEL_TO_FLOAT(GetGreen());
}

float Color::GetBlueF() const
{
  return CHANNEL_TO_FLOAT(GetBlue());
}

float Color::GetAlfaF() const
{
  return CHANNEL_TO_FLOAT(GetAlfa());
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

} // namespace dp
