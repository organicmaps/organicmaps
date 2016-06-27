#import "Common.h"
#import "MWMSearchHistoryRequestCell.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"

@interface MWMSearchHistoryRequestCell ()

@property (weak, nonatomic) IBOutlet UILabel * title;
@property (weak, nonatomic) IBOutlet UIImageView * icon;

@end

@implementation MWMSearchHistoryRequestCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  if (IPAD)
    self.contentView.backgroundColor = [UIColor white];
  self.layer.shouldRasterize = YES;
  self.layer.rasterizationScale = UIScreen.mainScreen.scale;
}

- (void)config:(NSString *)title
{
  self.title.text = title;
  [self.title sizeToFit];
  if (isIOS7)
    [self layoutIfNeeded];
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  if (isIOS7)
  {
    self.title.preferredMaxLayoutWidth = floor(self.title.width);
    [super layoutSubviews];
  }
}

+ (CGFloat)defaultCellHeight
{
  return 44.0;
}

- (CGFloat)cellHeight
{
  return ceil([self.contentView systemLayoutSizeFittingSize:UILayoutFittingCompressedSize].height);
}

@end
