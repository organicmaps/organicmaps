#import "MWMBottomMenuCollectionViewCell.h"
#import "SwiftBridge.h"

@interface MWMBottomMenuCollectionViewCell ()

@property(weak, nonatomic) IBOutlet UILabel * label;
@property(weak, nonatomic) IBOutlet UIView * badgeBackground;
@property(weak, nonatomic) IBOutlet UILabel * badgeCount;
@property(weak, nonatomic) IBOutlet UIView * separator;

@property(nonatomic, readwrite) BOOL isEnabled;
@property(nonatomic) BOOL isWideMenu;
@property(nonatomic) BOOL promo;

@end

@implementation MWMBottomMenuCollectionViewCell

- (void)configureWithImageName:(NSString *)imageName
                         label:(NSString *)label
                    badgeCount:(NSUInteger)badgeCount
                     isEnabled:(BOOL)isEnabled
{
  self.icon.image = [UIImage imageNamed:imageName];
  self.label.text = label;
  if (badgeCount > 0)
  {
    self.badgeBackground.hidden = NO;
    self.badgeCount.hidden = NO;
    self.badgeCount.text = @(badgeCount).stringValue;
  }
  else
  {
    self.badgeBackground.hidden = YES;
    self.badgeCount.hidden = YES;
  }
  self.isEnabled = isEnabled;
  [self.icon setStyleAndApply: isEnabled ? @"MWMBlack" : @"MWMGray" ];
  [self.label setStyleAndApply: isEnabled ? @"blackPrimaryText" : @"blackHintText"];
  self.promo = NO;
}

- (void)configurePromoWithImageName:(NSString *)imageName
                              label:(NSString *)label
{
  self.icon.image = [UIImage imageNamed:imageName];
  self.label.text = label;
  [self.icon setStyleAndApply:@"MWMBlue"];
  [self.label setStyleAndApply:@"linkBlueText"];
  self.badgeBackground.hidden = YES;
  self.badgeCount.hidden = YES;
  self.isEnabled = YES;
  self.promo = YES;
}

- (void)setHighlighted:(BOOL)highlighted
{
  if (!self.isEnabled)
    return;

  [super setHighlighted:highlighted];
  if (self.promo)
    [self.label setStyleAndApply: highlighted ? @"linkBlueHighlightedText" : @"linkBlueText"];
  else
    [self.label setStyleAndApply: highlighted ? @"blackHintText" : @"blackPrimaryText"];
}

- (void)setSelected:(BOOL)selected
{
  // There is no need to do something after cell has been selected.
}

@end

@implementation MWMBottomMenuCollectionViewPortraitCell
@end

@implementation MWMBottomMenuCollectionViewLandscapeCell
@end
