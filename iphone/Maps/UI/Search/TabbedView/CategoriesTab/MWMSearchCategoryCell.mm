#import "MWMSearchCategoryCell.h"
#import "MWMCommon.h"
#import "UIImageView+Coloring.h"

extern NSString * const kLuggageCategory;

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
  if ([category isEqualToString:kLuggageCategory])
  {
    label.text = L(@"luggage_storage");
    self.adIcon.hidden = NO;
    self.adIcon.image = [UIImage imageNamed:@"logo_luggage"];
  }
  else
  {
    label.text = L(category);
    self.adIcon.hidden = YES;
  }
}

@end
