#include "drape/color.hpp"

#include <algorithm>

namespace dp
{

bool HSL::AdjustLightness(bool isLightTheme)
{
  // Possible alternatives:
  // Inverse: l = 1.0f - l;
  // Also reduce saturaton: s = std::max(0, s - 0.1f);

  if (isLightTheme && l > 0.8f)
  {
    l = 0.2f;
    return true;
  }
  else if (!isLightTheme && l < 0.2f)
  {
    l = 0.8f;
    return true;
  }

  return false;
}

HSL Color2HSL(Color c)
{
  float const r = c.GetRedF();
  float const g = c.GetGreenF();
  float const b = c.GetBlueF();

  float const max = std::max({r, g, b});
  float const min = std::min({r, g, b});
  float const d = max - min;

  HSL hsl;
  hsl.l = (max + min) / 2.0f;

  if (d == 0)
    hsl.h = hsl.s = 0;  // gray
  else
  {
    hsl.s = hsl.l > 0.5f ? d / (2.0f - max - min) : d / (max + min);

    if (max == r)
      hsl.h = (g - b) / d + (g < b ? 6.0f : 0.0f);
    else if (max == g)
      hsl.h = (b - r) / d + 2.0f;
    else
      hsl.h = (r - g) / d + 4.0f;

    hsl.h *= 60.0f;
  }

  return hsl;
}

float hue2rgb(float p, float q, float t)
{
  if (t < 0.0f)
    t += 1.0f;
  if (t > 1.0f)
    t -= 1.0f;
  if (t < 1.0f / 6.0f)
    return p + (q - p) * 6.0f * t;
  if (t < 1.0f / 2.0f)
    return q;
  if (t < 2.0f / 3.0f)
    return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
  return p;
}

Color HSL2Color(HSL const & hsl)
{
  float const h = hsl.h / 360.0f;

  if (hsl.s == 0)
    return Color::FromFloat(hsl.l, hsl.l, hsl.l);  // gray
  else
  {
    float q = hsl.l < 0.5f ? hsl.l * (1.0f + hsl.s) : hsl.l + hsl.s - hsl.l * hsl.s;
    float p = 2.0f * hsl.l - q;
    return Color::FromFloat(hue2rgb(p, q, h + 1.0f / 3.0f), hue2rgb(p, q, h), hue2rgb(p, q, h - 1.0f / 3.0f));
  }
}

}  // namespace dp
