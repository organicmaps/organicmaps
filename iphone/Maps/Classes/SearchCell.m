
#import "SearchCell.h"
#import "UIKitCategories.h"

@interface SearchCell ()

@property (nonatomic) UIImageView * separatorView;

@end

@implementation SearchCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  self.backgroundColor = [UIColor clearColor];
  self.selectionStyle = UITableViewCellSelectionStyleDefault;
  [self addSubview:self.separatorView];

  UIView * selectedBackgroundView = [[UIView alloc] initWithFrame:self.bounds];
  selectedBackgroundView.backgroundColor = [UIColor colorWithColorCode:@"15d081"];
  self.selectedBackgroundView = selectedBackgroundView;

  return self;
}

- (void)layoutSubviews
{
  self.separatorView.maxY = self.height;
  CGFloat const shift = 15;
  self.separatorView.width = self.width - 2 * shift;
  self.separatorView.minX = shift;
  self.selectedBackgroundView.frame = self.bounds;
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
