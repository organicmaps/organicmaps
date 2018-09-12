#import "MWMSearchCategoryCell.h"
#import "MWMCommon.h"
#import "UIImageView+Coloring.h"

@interface MWMSearchCategoryCell ()

@property(weak, nonatomic) IBOutlet UIImageView * icon;
@property(weak, nonatomic) IBOutlet UILabel * label;
@property(weak, nonatomic) IBOutlet UIImageView * adIcon;

@end

@implementation MWMSearchCategoryCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  if (IPAD)
    self.contentView.backgroundColor = [UIColor white];
  CALayer * sl = self.layer;
  sl.shouldRasterize = YES;
  sl.rasterizationScale = UIScreen.mainScreen.scale;
}

- (void)setCategory:(NSString *)category
{
  UILabel * label = self.label;
  label.textColor = [UIColor blackPrimaryText];
  self.icon.mwm_name = [NSString stringWithFormat:@"ic_%@", category];
  label.text = L(category);
  self.adIcon.hidden = YES;
}

@end
