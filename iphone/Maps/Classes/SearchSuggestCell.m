
#import "SearchSuggestCell.h"
#import "UIKitCategories.h"

@interface SearchSuggestCell ()

@property (nonatomic) UILabel * titleLabel;
@property (nonatomic) UIImageView * iconImageView;

@end

@implementation SearchSuggestCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  [self.contentView addSubview:self.titleLabel];
  [self.contentView addSubview:self.iconImageView];

  UIView * backgroundView = [[UIView alloc] initWithFrame:self.bounds];
  backgroundView.backgroundColor = [UIColor colorWithColorCode:@"1f9f7e"];
  self.backgroundView = backgroundView;

  return self;
}

- (void)layoutSubviews
{
  [super layoutSubviews];

  self.titleLabel.width = self.width - self.titleLabel.minX - 20;
  self.titleLabel.minX = self.iconImageView.minX + self.iconImageView.width + 5;
  
  CGFloat const offset = 12.5;
  self.separatorView.width = self.width - 2 * offset;
  self.separatorView.minX = offset;
}

+ (CGFloat)cellHeight
{
  return 44;
}

- (UILabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(44, INTEGRAL(8.5), 0, 24)];
    _titleLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.textColor = [UIColor whiteColor];
    _titleLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:17.5];
  }
  return _titleLabel;
}

- (UIImageView *)iconImageView
{
  if (!_iconImageView)
  {
    _iconImageView = [[UIImageView alloc] initWithFrame:CGRectMake(16, 12, 20, 20)];
    _iconImageView.contentMode = UIViewContentModeCenter;
  }
  return _iconImageView;
}


@end
