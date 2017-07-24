#import "MWMSearchCategoryCell.h"
#import "MWMCommon.h"
#import "UIImageView+Coloring.h"

extern NSString * const kCianCategory;

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
  if ([category isEqualToString:kCianCategory])
  {
    label.text = L(@"real_estate");
    self.adIcon.hidden = NO;
    self.adIcon.mwm_name = @"logo_cian";
  }
  else
  {
    label.text = L(category);
    self.adIcon.hidden = YES;
  }
}

@end
