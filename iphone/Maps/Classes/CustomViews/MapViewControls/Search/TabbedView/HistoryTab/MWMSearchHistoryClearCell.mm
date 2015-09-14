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

#pragma mark - Properties

- (void)setIsLightTheme:(BOOL)isLightTheme
{
  _isLightTheme = isLightTheme;
  self.icon.image = [UIImage imageNamed:isLightTheme ? @"ic_clear_light" : @"ic_clear_dark"];
}

@end
