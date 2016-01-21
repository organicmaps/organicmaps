#import "MWMSearchHistoryClearCell.h"
#import "UIColor+MapsMeColor.h"

@interface MWMSearchHistoryClearCell ()

@property (weak, nonatomic) IBOutlet UIImageView * icon;

@end

@implementation MWMSearchHistoryClearCell

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
