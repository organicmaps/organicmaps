
#import "SearchShowOnMapCell.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"
#import "UIKitCategories.h"

static CGFloat const kOffset = 16.;

@interface SearchShowOnMapCell ()

@property (nonatomic) UILabel * titleLabel;

@end

@implementation SearchShowOnMapCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (self)
    [self.contentView addSubview:self.titleLabel];

  return self;
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

- (UILabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(kOffset, INTEGRAL(8.5), 0, 24)];
    _titleLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.textColor = [UIColor primary];
    _titleLabel.font = [UIFont regular16];
  }
  return _titleLabel;
}

@end
