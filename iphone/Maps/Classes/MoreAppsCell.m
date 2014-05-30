
#import "MoreAppsCell.h"
#import "UIKitCategories.h"

@interface MoreAppsCell ()

@property (nonatomic) UIButton * priceButton;

@end

@implementation MoreAppsCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  [self.contentView addSubview:self.priceButton];

  return self;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.priceButton.midY = self.height / 2;
  self.priceButton.maxX = self.width - 10;
}

- (void)setFree:(BOOL)free
{
  NSString * title = free ? NSLocalizedString(@"price_free", nil) : NSLocalizedString(@"price_paid", nil);
  title = [title uppercaseString];
  self.priceButton.width = [title sizeWithDrawSize:CGSizeMake(100, self.priceButton.height) font:self.priceButton.titleLabel.font].width + 18;
  [self.priceButton setTitle:title forState:UIControlStateNormal];
}

- (UIButton *)priceButton
{
  if (!_priceButton)
  {
    UIImage * image = [[UIImage imageNamed:@"PriceButton"] resizableImageWithCapInsets:UIEdgeInsetsMake(4, 4, 4, 4)];
    _priceButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 0, image.size.height)];
    _priceButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
    _priceButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Medium" size:12.5];
    _priceButton.userInteractionEnabled = NO;
    _priceButton.contentEdgeInsets = UIEdgeInsetsMake(0, 1, 0, 0);
    [_priceButton setBackgroundImage:image forState:UIControlStateNormal];
    [_priceButton setTitleColor:[UIColor colorWithColorCode:@"007aff"] forState:UIControlStateNormal];
  }
  return _priceButton;
}

@end
