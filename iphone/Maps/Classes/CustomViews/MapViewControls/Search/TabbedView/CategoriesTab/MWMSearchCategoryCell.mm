#import "Macros.h"
#import "MWMSearchCategoryCell.h"

@interface MWMSearchCategoryCell ()

@property (weak, nonatomic) IBOutlet UIImageView * icon;
@property (weak, nonatomic) IBOutlet UILabel * label;

@end

@implementation MWMSearchCategoryCell

- (void)awakeFromNib
{
  CALayer * sl = self.layer;
  sl.shouldRasterize = YES;
  sl.rasterizationScale = UIScreen.mainScreen.scale;
}

- (void)setCategory:(NSString *)category isLightTheme:(BOOL)isLightTheme;
{
  self.label.text = L(category);
  NSString * theme = isLightTheme ? @"light" : @"dark";
  NSString * imageName = [NSString stringWithFormat:@"ic_%@_%@", category, theme];
  self.icon.image = [UIImage imageNamed:imageName];
}

@end
