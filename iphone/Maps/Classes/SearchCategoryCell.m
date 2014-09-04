
#import "SearchCategoryCell.h"
#import "UIKitCategories.h"

@interface SearchCategoryCell ()

@property (nonatomic) UILabel * titleLabel;
@property (nonatomic) UIImageView * iconImageView;

@end

@implementation SearchCategoryCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  [self.contentView addSubview:self.titleLabel];
  [self.contentView addSubview:self.iconImageView];

  UIView * selectedBackgroundView = [[UIView alloc] initWithFrame:self.bounds];
  selectedBackgroundView.backgroundColor = [UIColor colorWithColorCode:@"15d081"];
  self.selectedBackgroundView = selectedBackgroundView;

  return self;
}

- (void)layoutSubviews
{
  [super layoutSubviews];

  self.titleLabel.width = self.width - self.titleLabel.minX - 20;

  CGFloat const offsetL = 40;
  CGFloat const offsetR = 12.5;
  self.separatorView.width = self.width - offsetL - offsetR;
  self.separatorView.minX = offsetL;
}

+ (CGFloat)cellHeight
{
  return 44;
}

- (UILabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(44, 8, 0, 24)];
    _titleLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.textColor = [UIColor whiteColor];
    _titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:17.5];
  }
  return _titleLabel;
}

- (UIImageView *)iconImageView
{
  if (!_iconImageView)
  {
    _iconImageView = [[UIImageView alloc] initWithFrame:CGRectMake(9, 9, 25, 25)];
    _iconImageView.contentMode = UIViewContentModeCenter;
  }
  return _iconImageView;
}

@end
