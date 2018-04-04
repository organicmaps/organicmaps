#import "MWMTableViewController.h"

#include "kml/types.hpp"

namespace ios_bookmark_ui_helper
{
inline NSString * TitleForBookmarkColor(kml::PredefinedColor color)
{
  switch (color)
  {
  case kml::PredefinedColor::Red: return @"red";
  case kml::PredefinedColor::Blue: return @"blue";
  case kml::PredefinedColor::Purple: return @"purple";
  case kml::PredefinedColor::Yellow: return @"yellow";
  case kml::PredefinedColor::Pink: return @"pink";
  case kml::PredefinedColor::Brown: return @"brown";
  case kml::PredefinedColor::Green: return @"green";
  case kml::PredefinedColor::Orange: return @"orange";
  case kml::PredefinedColor::None:
  case kml::PredefinedColor::Count: return @"";
  }
}

inline NSString * LocalizedTitleForBookmarkColor(kml::PredefinedColor color)
{
  return L(TitleForBookmarkColor(color));
}
  
inline UIImage * ImageForBookmarkColor(kml::PredefinedColor color, BOOL isSelected)
{
  return [UIImage imageNamed:[NSString stringWithFormat:@"%@%@%@", @"img_placemark-",
                              TitleForBookmarkColor(color), isSelected ? @"-on" : @"-off"]];
}

}  // namespace ios_bookmark_ui_helper

@protocol MWMBookmarkColorDelegate <NSObject>

- (void)didSelectColor:(kml::PredefinedColor)color;

@end

@interface MWMBookmarkColorViewController : MWMTableViewController

- (instancetype)initWithColor:(kml::PredefinedColor)color delegate:(id<MWMBookmarkColorDelegate>)delegate;

@end
