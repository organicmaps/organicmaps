#pragma once

#include "base/math.hpp"

#include "std/cstdint.hpp"
#include "std/sstream.hpp"
#include "std/string.hpp"

namespace dp
{

struct Color
{
  Color();
  explicit Color(uint32_t rgba);
  Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);

  uint8_t GetRed() const;
  uint8_t GetGreen() const;
  uint8_t GetBlue() const;
  uint8_t GetAlpha() const;

  float GetRedF() const;
  float GetGreenF() const;
  float GetBlueF() const;
  float GetAlphaF() const;

  bool operator==(Color const & other) const { return m_rgba == other.m_rgba; }
  bool operator!=(Color const & other) const { return m_rgba != other.m_rgba; }
  bool operator< (Color const & other) const { return m_rgba < other.m_rgba; }
  Color operator*(float s) const
  {
    return Color(static_cast<uint8_t>(my::clamp(GetRedF() * s, 0.0f, 1.0f) * 255.0f),
                 static_cast<uint8_t>(my::clamp(GetGreenF() * s, 0.0f, 1.0f) * 255.0f),
                 static_cast<uint8_t>(my::clamp(GetBlueF() * s, 0.0f, 1.0f) * 255.0f),
                 GetAlpha());
  }

  static Color Black()       { return Color(0, 0, 0, 255); }
  static Color White()       { return Color(255, 255, 255, 255); }
  static Color Red()         { return Color(255, 0, 0, 255); }
  static Color Green()       { return Color(0, 255, 0, 255); }
  static Color Yellow()       { return Color(255, 255, 0, 255); }
  static Color Transparent() { return Color(0, 0, 0, 0); }

private:
  uint32_t m_rgba;
};

inline uint8_t ExtractRed(uint32_t argb);
inline uint8_t ExtractGreen(uint32_t argb);
inline uint8_t ExtractBlue(uint32_t argb);
inline uint8_t ExtractAlpha(uint32_t argb);
Color Extract(uint32_t argb);
Color Extract(uint32_t xrgb, uint8_t a);

inline string DebugPrint(Color const & c)
{
  ostringstream out;
  out << "[R = " << static_cast<uint32_t>(c.GetRed())
      << ", G = " << static_cast<uint32_t>(c.GetGreen())
      << ", B = " << static_cast<uint32_t>(c.GetBlue())
      << ", A = " << static_cast<uint32_t>(c.GetAlpha()) << "]";
  return out.str();
}

}
