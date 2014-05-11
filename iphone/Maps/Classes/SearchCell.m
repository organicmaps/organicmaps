
#import "SearchCell.h"
#import "UIKitCategories.h"

@interface SearchCell ()

@property (nonatomic) UIView * separatorView;

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
  self.selectedBackgroundView.height = self.height;
}

- (UIView *)separatorView
{
  if (!_separatorView)
  {
    _separatorView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, self.width, 0.5)];
    _separatorView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _separatorView.backgroundColor = [UIColor colorWithWhite:0.5 alpha:0.5];
  }
  return _separatorView;
}

@end
