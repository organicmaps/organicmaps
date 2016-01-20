#import "MWMSearchShowOnMapCell.h"
#import "UIColor+MapsMeColor.h"

@implementation MWMSearchShowOnMapCell

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
  return 44.0;
}

@end
