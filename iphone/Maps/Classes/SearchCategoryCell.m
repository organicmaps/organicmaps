
#import "SearchCategoryCell.h"
#import "UIKitCategories.h"
#import "UIColor+MapsMeColor.h"

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

  return self;
}

- (void)layoutSubviews
{
  [super layoutSubviews];

  CGFloat const offsetL = 56;
  self.titleLabel.origin = CGPointMake(offsetL, 8);
  self.titleLabel.width = self.width - self.titleLabel.minX - 20;
  self.titleLabel.center = CGPointMake(self.titleLabel.center.x, self.height / 2.);
  self.iconImageView.center = CGPointMake(self.iconImageView.center.x, self.titleLabel.center.y);

  self.separatorView.width = self.width - offsetL;
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
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 0, 24)];
    _titleLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.textColor = [UIColor blackPrimaryText];
    _titleLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:16.];
  }
  return _titleLabel;
}

- (UIImageView *)iconImageView
{
  if (!_iconImageView)
  {
    _iconImageView = [[UIImageView alloc] initWithFrame:CGRectMake(16, 9, 25, 25)];
    _iconImageView.contentMode = UIViewContentModeCenter;
  }
  return _iconImageView;
}

@end
