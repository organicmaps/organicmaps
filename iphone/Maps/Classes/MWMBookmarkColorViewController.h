#import "MWMTableViewController.h"

namespace ios_bookmark_ui_helper
{
inline NSString * LocalizedTitleForBookmarkColor(NSString * color)
{
  return L([color stringByReplacingOccurrencesOfString:@"placemark-" withString:@""]);
}

inline UIImage * ImageForBookmarkColor(NSString * color, BOOL isSelected)
{
  return [UIImage imageNamed:[NSString stringWithFormat:@"%@%@%@", @"img_", color, isSelected ? @"-on" : @"-off"]];
}

}  // namespace ios_bookmark_ui_helper

@protocol MWMBookmarkColorDelegate <NSObject>

- (void)didSelectColor:(NSString *)color;

@end

@interface MWMBookmarkColorViewController : MWMTableViewController

- (instancetype)initWithColor:(NSString *)color delegate:(id<MWMBookmarkColorDelegate>)delegate;

@end
