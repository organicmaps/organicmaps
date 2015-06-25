
#import "SearchCell.h"
#import "UIKitCategories.h"
#import "UIColor+MapsMeColor.h"

@implementation SearchCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  self.backgroundColor = [UIColor clearColor];
  self.selectionStyle = UITableViewCellSelectionStyleDefault;
  [self addSubview:self.separatorView];
  UIView * selectedView = [[UIView alloc] initWithFrame:self.bounds];
  selectedView.backgroundColor = [UIColor pressBackground];
  self.selectedBackgroundView = selectedView;
  return self;
}

- (void)layoutSubviews
{
  self.separatorView.maxY = self.height;
  self.selectedBackgroundView.frame = self.bounds;
  self.backgroundView.frame = self.bounds;
}

- (UIView *)separatorView
{
  if (!_separatorView)
  {
    _separatorView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, self.width, 1)];
    _separatorView.backgroundColor = [UIColor blackDividers];
    _separatorView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  }
  return _separatorView;
}

@end
