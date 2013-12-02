
#import "SideToolbarCell.h"
#import "UIKitCategories.h"

@interface SideToolbarCell ()

@property (nonatomic) UIImageView * separator;
@property (nonatomic) UIImageView * proMarker;

@end

@implementation SideToolbarCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  self.backgroundColor = [UIColor clearColor];

  self.iconImageView.origin = CGPointMake(8, 6.5);

  UIView * highlightView = [[UIView alloc] initWithFrame:self.bounds];
  highlightView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.25];
  self.selectedBackgroundView = highlightView;

  [self.contentView addSubview:self.titleLabel];
  [self.contentView addSubview:self.iconImageView];
  [self.contentView addSubview:self.proMarker];
  [self.contentView addSubview:self.separator];

  self.titleLabel.textColor = [UIColor whiteColor];

  return self;
}

+ (CGFloat)cellHeight
{
  return 55;
}

- (void)layoutSubviews
{
  [super layoutSubviews];

  CGFloat width;
  if (SYSTEM_VERSION_IS_LESS_THAN(@"7"))
  {
    width = [self.titleLabel.text sizeWithFont:self.titleLabel.font].width;
  }
  else
  {
    CGRect rect = [self.titleLabel.text boundingRectWithSize:self.titleLabel.frame.size
                                                     options:NSStringDrawingUsesLineFragmentOrigin
                                                  attributes:@{NSFontAttributeName : self.titleLabel.font}
                                                     context:nil];
    width = rect.size.width;
  }
  self.proMarker.origin = CGPointMake(width + 60, 12.5);
  self.separator.minY = self.height - self.separator.height;
  CGFloat shift = 8;
  self.separator.minX = shift;
  self.separator.width = self.width - 2 * shift;

  self.titleLabel.midY = self.height / 2 - 2;
  self.titleLabel.minX = 53;
}

- (void)setDisabled:(BOOL)disabled
{
  self.proMarker.hidden = !disabled;
  self.titleLabel.width = disabled ? 140 : 200;
  _disabled = disabled;
}

- (UIImageView *)separator
{
  if (!_separator)
  {
    UIImage * separatorImage = [[UIImage imageNamed:@"side-toolbar-cell-separator"] resizableImageWithCapInsets:UIEdgeInsetsZero];
    _separator = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, self.width, separatorImage.size.height)];
    _separator.image = separatorImage;
  }
  return _separator;
}

- (UIImageView *)proMarker
{
  if (!_proMarker)
  {
    _proMarker = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"side-toolbar-cell-background-pro"]];
    UILabel * label = [[UILabel alloc] initWithFrame:_proMarker.bounds];
    label.minY = -0.5;
    label.backgroundColor = [UIColor clearColor];
    label.font = [UIFont fontWithName:@"HelveticaNeue-Bold" size:12.5];
    label.textColor = [UIColor whiteColor];
    label.text = @"PRO";
    label.textAlignment = NSTextAlignmentCenter;
    [_proMarker addSubview:label];
  }
  return _proMarker;
}

- (UIImageView *)iconImageView
{
  if (!_iconImageView)
  {
    _iconImageView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 40, 40)];
    _iconImageView.contentMode = UIViewContentModeCenter;
  }
  return _iconImageView;
}

- (UILabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 0, 48)];
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:17.5];
  }
  return _titleLabel;
}

@end
