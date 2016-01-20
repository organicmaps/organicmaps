#import "MWMSearchHistoryMyPositionCell.h"
#import "UIColor+MapsMeColor.h"

@implementation MWMSearchHistoryMyPositionCell

- (void)awakeFromNib
{
  if (IPAD)
    self.contentView.backgroundColor = [UIColor white];
  CALayer * sl = self.layer;
  sl.shouldRasterize = YES;
  sl.rasterizationScale = UIScreen.mainScreen.scale;
}

+ (CGFloat)cellHeight
{
  return 44.;
}

@end
