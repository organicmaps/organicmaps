#import "CircleView.h"

#include "kml/types.hpp"

namespace ios_bookmark_ui_helper
{
inline UIColor * UIColorForRGB(int red, int green, int blue)
{
  return [UIColor colorWithRed:red / 255.f green:green / 255.f blue:blue / 255.f alpha:0.8];
}

inline UIColor * UIColorForBookmarkColor(kml::PredefinedColor color)
{
  auto const dpColor = kml::ColorFromPredefinedColor(color);
  return UIColorForRGB(dpColor.red, dpColor.green, dpColor.blue);
}

inline UIImage * ImageForBookmark(kml::PredefinedColor color, kml::BookmarkIcon icon)
{
  CGFloat const kPinDiameter = 22;

  NSString * imageName =
      [NSString stringWithFormat:@"%@%@", @"ic_bm_", [@(kml::ToString(icon).c_str()) lowercaseString]];

  return [CircleView createCircleImageWithDiameter:kPinDiameter
                                          andColor:UIColorForBookmarkColor(color)
                                      andImageName:imageName];
}

inline UIImage * ImageForTrack(float red, float green, float blue)
{
  CGFloat const kPinDiameter = 22;
  return [CircleView createCircleImageWithDiameter:kPinDiameter
                                          andColor:[UIColor colorWithRed:red green:green blue:blue alpha:1.f]];
}
}  // namespace ios_bookmark_ui_helper
