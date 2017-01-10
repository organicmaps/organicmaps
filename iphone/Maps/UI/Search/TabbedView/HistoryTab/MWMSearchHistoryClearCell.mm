#import "MWMSearchHistoryClearCell.h"

@interface MWMSearchHistoryClearCell ()

@property (weak, nonatomic) IBOutlet UIImageView * icon;

@end

@implementation MWMSearchHistoryClearCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  if (IPAD)
    self.contentView.backgroundColor = [UIColor white];
  CALayer * sl = self.layer;
  sl.shouldRasterize = YES;
  sl.rasterizationScale = UIScreen.mainScreen.scale;
}

@end
