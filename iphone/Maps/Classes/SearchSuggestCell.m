
#import "SearchSuggestCell.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"
#import "UIKitCategories.h"

@interface SearchSuggestCell ()

@property (nonatomic) UIImageView * iconImageView;

@end

@implementation SearchSuggestCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (self)
  {
    [self.contentView addSubview:self.titleLabel];
    [self.contentView addSubview:self.iconImageView];
  }

  return self;
}

- (void)configTitleLabel
{
  [super configTitleLabel];
  self.titleLabel.frame = CGRectMake(0., INTEGRAL(8.5), 0, 24);
  self.titleLabel.textColor = [UIColor primary];
}

- (NSDictionary *)selectedTitleAttributes
{
  static NSDictionary * selectedAttributes;
  if (!selectedAttributes)
    selectedAttributes = @{NSForegroundColorAttributeName : [UIColor primary], NSFontAttributeName : [UIFont bold16]};
  return selectedAttributes;
}

- (NSDictionary *)unselectedTitleAttributes
{
  static NSDictionary * unselectedAttributes;
  if (!unselectedAttributes)
    unselectedAttributes = @{NSForegroundColorAttributeName : [UIColor primary], NSFontAttributeName : [UIFont regular16]};
  return unselectedAttributes;
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
