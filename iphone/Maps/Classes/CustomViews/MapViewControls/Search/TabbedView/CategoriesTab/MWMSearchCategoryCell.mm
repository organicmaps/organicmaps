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

- (void)setCategory:(NSString *)category
{
  self.label.text = L(category);
  self.icon.image = [UIImage imageNamed:[NSString stringWithFormat:@"ic_%@_light", category]];
}

@end
