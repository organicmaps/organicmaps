
#import "SearchSuggestCell.h"
#import "UIKitCategories.h"
#import "UIColor+MapsMeColor.h"

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
  return self;
}

- (void)layoutSubviews
{
  [super layoutSubviews];

  CGFloat const offset = 34.;
  self.titleLabel.minX = offset;
  self.titleLabel.width = self.width - self.titleLabel.minX - offset;
  self.titleLabel.center = CGPointMake(self.titleLabel.center.x, self.height / 2.);
  self.iconImageView.center = CGPointMake(self.iconImageView.center.x, self.titleLabel.center.y);

  
  self.separatorView.width = self.width - 16.;
  self.separatorView.minX = 16.;
}

+ (CGFloat)cellHeight
{
  return 44;
}

- (UILabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(0., INTEGRAL(8.5), 0, 24)];
    _titleLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.textColor = [UIColor primary];
    _titleLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:16.];
  }
  return _titleLabel;
}

- (UIImageView *)iconImageView
{
  if (!_iconImageView)
  {
    _iconImageView = [[UIImageView alloc] initWithFrame:CGRectMake(16, 14, 12, 12)];
    _iconImageView.image = [UIImage imageNamed:@"ic_suggest"];
    _iconImageView.contentMode = UIViewContentModeCenter;
  }
  return _iconImageView;
}


@end
