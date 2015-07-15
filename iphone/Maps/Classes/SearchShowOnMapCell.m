
#import "SearchShowOnMapCell.h"
#import "UIColor+MapsMeColor.h"
#import "UIKitCategories.h"

static CGFloat const kOffset = 16.;

@implementation SearchShowOnMapCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (self)
    [self.contentView addSubview:self.titleLabel];

  return self;
}

- (void)configTitleLabel
{
  [super configTitleLabel];
  self.titleLabel.frame = CGRectMake(kOffset, INTEGRAL(8.5), 0, 24);
  self.titleLabel.textColor = [UIColor primary];
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.titleLabel.width = self.width - self.titleLabel.minX - kOffset;
  self.titleLabel.minX = kOffset;
  self.separatorView.width = self.width;
  self.separatorView.minX = 0;
  self.separatorView.minY = self.height - 1;
}

+ (CGFloat)cellHeight
{
  return 44;
}

@end
