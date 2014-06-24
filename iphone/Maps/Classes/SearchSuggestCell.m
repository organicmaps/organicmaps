
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

  self.backgroundColor = [UIColor clearColor];

  UIImageView * backgroundImageView = [[UIImageView alloc] initWithFrame:self.bounds];
  self.backgroundView = backgroundImageView;

  UIImageView * selectedBackgroundImageView = [[UIImageView alloc] initWithFrame:self.bounds];
  self.selectedBackgroundView = selectedBackgroundImageView;

  return self;
}

- (void)layoutSubviews
{
  UIImage * image;
  UIImage * selectedImage;
  if (self.position == SearchSuggestCellPositionMiddle)
  {
    image = [UIImage imageNamed:@"SearchSuggestBackgroundMiddle"];
    selectedImage = [UIImage imageNamed:@"SearchSuggestSelectedBackgroundMiddle"];
  }
  else
  {
    image = [UIImage imageNamed:@"SearchSuggestBackgroundBottom"];
    selectedImage = [UIImage imageNamed:@"SearchSuggestSelectedBackgroundBottom"];
  }

  UIEdgeInsets insets = UIEdgeInsetsMake(10, 40, 10, 40);

  ((UIImageView *)self.backgroundView).image = [image resizableImageWithCapInsets:insets];
  self.backgroundView.frame = self.bounds;

  ((UIImageView *)self.selectedBackgroundView).image = [selectedImage resizableImageWithCapInsets:insets];
  self.selectedBackgroundView.frame = self.bounds;

  self.titleLabel.width = self.width - self.titleLabel.minX - 20;
}

+ (CGFloat)cellHeightWithPosition:(SearchSuggestCellPosition)position
{
  return position == SearchSuggestCellPositionMiddle ? 44 : 46;
}

- (UILabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(44, 8.5, 0, 24)];
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
    _iconImageView = [[UIImageView alloc] initWithFrame:CGRectMake(17, 13, 18, 18)];
    _iconImageView.contentMode = UIViewContentModeCenter;
  }
  return _iconImageView;
}


@end
