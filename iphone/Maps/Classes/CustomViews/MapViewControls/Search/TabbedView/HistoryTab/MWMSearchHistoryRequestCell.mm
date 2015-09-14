#import "MWMSearchHistoryRequestCell.h"
#import "UIFont+MapsMeFonts.h"

static CGFloat kPreferredMaxLayoutWidth = 244.0;

@interface MWMSearchHistoryRequestCell ()

@property (weak, nonatomic) IBOutlet UILabel * title;
@property (weak, nonatomic) IBOutlet UIImageView * icon;

@end

@implementation MWMSearchHistoryRequestCell

- (void)awakeFromNib
{
  self.layer.shouldRasterize = YES;
  self.layer.rasterizationScale = UIScreen.mainScreen.scale;
}

- (void)config:(NSString *)title
{
  self.title.text = title;
  self.title.preferredMaxLayoutWidth = kPreferredMaxLayoutWidth;
  [self.title sizeToFit];
  [self setNeedsLayout];
  [self layoutIfNeeded];
}

+ (CGFloat)defaultCellHeight
{
  return 44.0;
}

- (CGFloat)cellHeight
{
  return ceil([self.contentView systemLayoutSizeFittingSize:UILayoutFittingCompressedSize].height);
}

#pragma mark - Properties

- (void)setIsLightTheme:(BOOL)isLightTheme
{
  _isLightTheme = isLightTheme;
  self.icon.image =
      [UIImage imageNamed:isLightTheme ? @"ic_history_label_light" : @"ic_history_label_dark"];
}

@end
