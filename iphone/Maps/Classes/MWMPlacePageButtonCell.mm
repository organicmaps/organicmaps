#import "MWMPlacePage.h"
#import "MWMPlacePageButtonCell.h"
#import "Statistics.h"

@interface MWMPlacePageButtonCell ()

@property (weak, nonatomic) MWMPlacePage * placePage;

@end

@implementation MWMPlacePageButtonCell

+ (CGFloat)height
{
  return 44.0;
}

- (void)config:(MWMPlacePage *)placePage
{
  self.placePage = placePage;
}

- (IBAction)editPlaceButtonTouchUpIndide
{
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatEdit)];
}

@end
