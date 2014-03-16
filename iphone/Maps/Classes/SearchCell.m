
#import "SearchCell.h"
#import "UIKitCategories.h"

@interface SearchCell ()

@property (nonatomic) UIImageView * separatorImageView;
@property (nonatomic) UIImageView * backgroundImageView;

@end

@implementation SearchCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  self.backgroundColor = [UIColor clearColor];
  self.selectionStyle = UITableViewCellSelectionStyleNone;
  [self addSubview:self.backgroundImageView];
  [self.contentView addSubview:self.separatorImageView];

  return self;
}

- (void)setPosition:(SearchCellPosition)position
{
  if (position == _position)
    return;

  UIImage * image;
  switch (position)
  {
  case SearchCellPositionFirst:
    image = [UIImage imageNamed:@"SearchCellBackgroundTop"];
    break;
  case SearchCellPositionMiddle:
    image = [UIImage imageNamed:@"SearchCellBackgroundMiddle"];
    break;
  case SearchCellPositionLast:
    image = [UIImage imageNamed:@"SearchCellBackgroundBottom"];
    break;
  case SearchCellPositionAlone:
    image = [UIImage imageNamed:@"SearchCellBackgroundAlone"];
    break;
  }
  self.backgroundImageView.image = [image resizableImageWithCapInsets:UIEdgeInsetsMake(6, 6, 6, 6)];

  _position = position;
}

- (void)setHighlighted:(BOOL)highlighted animated:(BOOL)animated
{
  [super setHighlighted:highlighted animated:animated];
  self.backgroundImageView.alpha = highlighted ? 0.5 : 1;
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated
{
  [super setSelected:selected animated:animated];
  self.backgroundImageView.alpha = selected ? 0.5 : 1;
}

- (void)layoutSubviews
{
  self.backgroundImageView.size = CGSizeMake(self.width - 28, self.height);
  self.backgroundImageView.midX = self.width / 2;
  [self sendSubviewToBack:self.backgroundImageView];

  self.contentView.frame = self.backgroundImageView.frame;

  self.separatorImageView.width = self.contentView.width;
  self.separatorImageView.maxY = self.contentView.height;
  self.separatorImageView.midX = self.contentView.width / 2;
  self.separatorImageView.hidden = (self.position == SearchCellPositionLast || self.position == SearchCellPositionAlone);
}

- (UIImageView *)backgroundImageView
{
  if (!_backgroundImageView)
  {
    _backgroundImageView = [[UIImageView alloc] initWithFrame:CGRectZero];
    _backgroundImageView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  }
  return _backgroundImageView;
}

- (UIImageView *)separatorImageView
{
  if (!_separatorImageView)
  {
    UIImage * image = [[UIImage imageNamed:@"SearchCellSeparator"] resizableImageWithCapInsets:UIEdgeInsetsMake(0, 40, 0, 40)];
    _separatorImageView = [[UIImageView alloc] initWithImage:image];
    _separatorImageView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  }
  return _separatorImageView;
}

@end
