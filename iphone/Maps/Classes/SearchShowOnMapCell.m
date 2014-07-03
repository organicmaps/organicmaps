
#import "SearchShowOnMapCell.h"
#import "UIKitCategories.h"

@interface SearchShowOnMapCell ()

@property (nonatomic) UILabel * titleLabel;

@end

@implementation SearchShowOnMapCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  [self.contentView addSubview:self.titleLabel];

  UIView * backgroundView = [[UIView alloc] initWithFrame:self.bounds];
  backgroundView.backgroundColor = [UIColor colorWithColorCode:@"1f9f7e"];
  self.backgroundView = backgroundView;

  self.separatorView.hidden = YES;

  return self;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.titleLabel.width = self.width - self.titleLabel.minX - 20;
}

+ (CGFloat)cellHeight
{
  return 44;
}

- (UILabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(20, 8.5, 0, 24)];
    _titleLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.textColor = [UIColor whiteColor];
    _titleLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:17.5];
  }
  return _titleLabel;
}

@end
