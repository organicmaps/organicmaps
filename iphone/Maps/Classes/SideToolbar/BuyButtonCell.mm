
#import "BuyButtonCell.h"
#import "UIKitCategories.h"
#include "../../../../platform/platform.hpp"

@interface BuyButtonCell ()

@property (nonatomic) UIButton * buyButton;

@end

@implementation BuyButtonCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

  self.selectionStyle = UITableViewCellSelectionStyleNone;
  self.backgroundColor = [UIColor clearColor];

  if (!GetPlatform().IsPro())
    [self.contentView addSubview:self.buyButton];

  return self;
}

- (void)layoutSubviews
{
  self.buyButton.center = CGPointMake(self.width / 2, self.height / 2);
}

- (void)buyButtonPressed:(id)sender
{
  [self.delegate buyButtonCellDidPressBuyButton:self];
}

+ (CGFloat)cellHeight
{
  return GetPlatform().IsPro() ? 0 : (IPAD ? 92 : 80);
}

- (UIButton *)buyButton
{
  if (!_buyButton)
  {
    UIImage * buyImage = [[UIImage imageNamed:@"ButtonBecomePro"] resizableImageWithCapInsets:UIEdgeInsetsMake(10, 10, 10, 10)];
    _buyButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, buyImage.size.width, buyImage.size.height + 6)];
    _buyButton.titleEdgeInsets = UIEdgeInsetsMake(0, 0, 2, 0);
    _buyButton.titleLabel.lineBreakMode = NSLineBreakByWordWrapping;
    _buyButton.titleLabel.textAlignment = NSTextAlignmentCenter;
    _buyButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:20];
    [_buyButton setBackgroundImage:buyImage forState:UIControlStateNormal];

    NSString * proText = NSLocalizedString(@"become_a_pro", nil);
    NSDictionary * attributes = @{ NSForegroundColorAttributeName : [UIColor whiteColor] };
    NSAttributedString * attributedProText = [[NSAttributedString alloc] initWithString:proText attributes:attributes];
    [_buyButton setAttributedTitle:attributedProText forState:UIControlStateNormal];

    [_buyButton addTarget:self action:@selector(buyButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _buyButton;
}

@end
