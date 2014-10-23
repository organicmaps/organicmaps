
#import "BottomMenuCell.h"
#import "UIKitCategories.h"

@interface BottomMenuCell ()

@property (nonatomic) UIImageView * separator;
@property (nonatomic) UIImageView * iconImageView;
@property (nonatomic) UILabel * titleLabel;
@property (nonatomic) BadgeView * badgeView;

@end

@implementation BottomMenuCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  self.backgroundColor = [UIColor clearColor];

  UIView * highlightView = [[UIView alloc] initWithFrame:self.bounds];
  highlightView.backgroundColor = [UIColor colorWithColorCode:@"15d081"];
  self.selectedBackgroundView = highlightView;

  [self.contentView addSubview:self.titleLabel];
  [self.contentView addSubview:self.iconImageView];
  [self.contentView addSubview:self.separator];
  [self.contentView addSubview:self.badgeView];

  return self;
}

+ (CGFloat)cellHeight
{
  return 50;
}

- (void)prepareForReuse
{
  self.separator.hidden = NO;
}

- (void)layoutSubviews
{
  self.selectedBackgroundView.frame = self.bounds;

  CGFloat shift = 19;
  self.separator.minX = shift;
  self.separator.width = self.width - 2 * shift;
  self.separator.maxY = self.height;

  self.titleLabel.size = CGSizeMake(self.width - 70, self.height);
  [self.titleLabel sizeToIntegralFit];
  self.titleLabel.minX = 53;
  self.titleLabel.midY = self.height / 2 - 2;

  self.badgeView.minX = self.titleLabel.maxX + 3;
  self.badgeView.minY = self.titleLabel.minY - 5;

  self.iconImageView.origin = CGPointMake(19, INTEGRAL(4.5));
}

- (UIImageView *)separator
{
  if (!_separator)
  {
    UIImage * separatorImage = [[UIImage imageNamed:@"SearchCellSeparator"] resizableImageWithCapInsets:UIEdgeInsetsZero];
    _separator = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, self.width, 1)];
    _separator.image = separatorImage;
  }
  return _separator;
}

- (UIImageView *)iconImageView
{
  if (!_iconImageView)
  {
    _iconImageView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 23, 41)];
    _iconImageView.contentMode = UIViewContentModeCenter;
  }
  return _iconImageView;
}

- (UILabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:17.5];
  }
  return _titleLabel;
}

- (BadgeView *)badgeView
{
  if (!_badgeView)
    _badgeView = [[BadgeView alloc] init];
  return _badgeView;
}

@end
