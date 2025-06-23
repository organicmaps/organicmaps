#pragma once
#import <UIKit/UIKit.h>

#include "drape/color.hpp"

static inline uint8_t ConvertColorComponentToHex(CGFloat color) {
  ASSERT_LESS_OR_EQUAL(color, 1.f, ("Extended sRGB color space is not supported"));
  ASSERT_GREATER_OR_EQUAL(color, 0.f, ("Extended sRGB color space is not supported"));
  static constexpr uint8_t kMaxChannelValue = 255;
  return color * kMaxChannelValue;
}

static inline dp::Color GetColorFromUIColor(UIColor * color) {
  CGFloat fRed, fGreen, fBlue, fAlpha;
  [color getRed:&fRed green:&fGreen blue:&fBlue alpha:&fAlpha];

  const uint8_t red = ConvertColorComponentToHex(fRed);
  const uint8_t green = ConvertColorComponentToHex(fGreen);
  const uint8_t blue = ConvertColorComponentToHex(fBlue);
  const uint8_t alpha = ConvertColorComponentToHex(fAlpha);

  return dp::Color(red, green, blue, alpha);
}
