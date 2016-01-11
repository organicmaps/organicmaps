#import "MWMSearchHistoryClearCell.h"

@interface MWMSearchHistoryClearCell ()

@property (weak, nonatomic) IBOutlet UIImageView * icon;

@end

@implementation MWMSearchHistoryClearCell

- (void)awakeFromNib
{
  CALayer * sl = self.layer;
  sl.shouldRasterize = YES;
  sl.rasterizationScale = UIScreen.mainScreen.scale;
}

+ (CGFloat)cellHeight
{
  return 44.0;
}

@end
