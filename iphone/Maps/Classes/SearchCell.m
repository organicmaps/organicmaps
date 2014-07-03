
#import "SearchCell.h"
#import "UIKitCategories.h"

@implementation SearchCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  self.backgroundColor = [UIColor clearColor];
  self.selectionStyle = UITableViewCellSelectionStyleDefault;
  [self addSubview:self.separatorView];

  UIImageView * selectedBackgroundView = [[UIImageView alloc] initWithFrame:self.bounds];
  selectedBackgroundView.backgroundColor = [UIColor colorWithWhite:1 alpha:0.2];
  self.selectedBackgroundView = selectedBackgroundView;

  return self;
}

- (void)layoutSubviews
{
  self.separatorView.maxY = self.height;
  self.selectedBackgroundView.frame = self.bounds;
  self.backgroundView.frame = self.bounds;
}

- (UIImageView *)separatorView
{
  if (!_separatorView)
  {
    _separatorView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, self.width, 1)];
    _separatorView.image = [[UIImage imageNamed:@"SearchCellSeparator"] resizableImageWithCapInsets:UIEdgeInsetsZero];
    _separatorView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  }
  return _separatorView;
}

@end
