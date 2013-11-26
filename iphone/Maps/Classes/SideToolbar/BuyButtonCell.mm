
#import "BuyButtonCell.h"
#import "UIKitCategories.h"
#include "../../platform/platform.hpp"

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
  return GetPlatform().IsPro() ? 0 : 70;
}

- (UIButton *)buyButton
{
  if (!_buyButton)
  {
    UIImage * buyImage = [UIImage imageNamed:@"side-toolbar-button-become-pro"];
    _buyButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, buyImage.size.width, buyImage.size.height)];
    _buyButton.titleEdgeInsets = UIEdgeInsetsMake(0, 0, 1, 0);
    _buyButton.contentMode = UIViewContentModeCenter;
    [_buyButton setBackgroundImage:buyImage forState:UIControlStateNormal];
    [_buyButton setTitle:NSLocalizedString(@"become_a_pro", nil) forState:UIControlStateNormal];
    [_buyButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [_buyButton addTarget:self action:@selector(buyButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _buyButton;
}

@end
