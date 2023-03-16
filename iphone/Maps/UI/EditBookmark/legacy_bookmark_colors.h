#import "CircleView.h"

#include "kml/types.hpp"

namespace ios_bookmark_ui_helper
{
inline UIColor * UIColorForRGB(int red, int green, int blue)
{
  return [UIColor colorWithRed:red/255.f green:green/255.f blue:blue/255.f alpha:0.8];
}

inline UIColor * UIColorForBookmarkColor(kml::PredefinedColor color)
{
  switch (color)
  {
    case kml::PredefinedColor::Red: return UIColorForRGB(229, 27, 35);
    case kml::PredefinedColor::Pink: return UIColorForRGB(255, 65, 130);
    case kml::PredefinedColor::Purple: return UIColorForRGB(155, 36, 178);
    case kml::PredefinedColor::DeepPurple: return UIColorForRGB(102, 57, 191);
    case kml::PredefinedColor::Blue: return UIColorForRGB(0, 102, 204);
    case kml::PredefinedColor::LightBlue: return UIColorForRGB(36, 156, 242);
    case kml::PredefinedColor::Cyan: return UIColorForRGB(20, 190, 205);
    case kml::PredefinedColor::Teal: return UIColorForRGB(0, 165, 140);
    case kml::PredefinedColor::Green: return UIColorForRGB(60, 140, 60);
    case kml::PredefinedColor::Lime: return UIColorForRGB(147, 191, 57);
    case kml::PredefinedColor::Yellow: return UIColorForRGB(255, 200, 0);
    case kml::PredefinedColor::Orange: return UIColorForRGB(255, 150, 0);
    case kml::PredefinedColor::DeepOrange: return UIColorForRGB(240, 100, 50);
    case kml::PredefinedColor::Brown: return UIColorForRGB(128, 70, 51);
    case kml::PredefinedColor::Gray: return UIColorForRGB(115, 115, 115);
    case kml::PredefinedColor::BlueGray: return UIColorForRGB(89, 115, 128);
    case kml::PredefinedColor::None:
    case kml::PredefinedColor::Count: return UIColorForBookmarkColor(kml::PredefinedColor::Red);
  }
}

inline UIImage * ImageForBookmark(kml::PredefinedColor color, kml::BookmarkIcon icon)
{
  CGFloat const kPinDiameter = 22;

  NSString *imageName = [NSString stringWithFormat:@"%@%@", @"ic_bm_", [@(kml::ToString(icon).c_str()) lowercaseString]];

  return [CircleView createCircleImageWithDiameter:kPinDiameter
                                          andColor:UIColorForBookmarkColor(color)
                                      andImageName:imageName];
}

inline UIImage * ImageForTrack(float red, float green, float blue)
{
  CGFloat const kPinDiameter = 22;
  return [CircleView createCircleImageWithDiameter:kPinDiameter
                                          andColor:[UIColor colorWithRed:red
                                                                   green:green
                                                                    blue:blue
                                                                   alpha:1.f]];
}
}  // namespace ios_bookmark_ui_helper

