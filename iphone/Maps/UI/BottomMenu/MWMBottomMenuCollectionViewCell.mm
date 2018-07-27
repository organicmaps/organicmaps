#import "MWMBottomMenuCollectionViewCell.h"
#import "MWMCommon.h"
#import "SwiftBridge.h"
#import "UIImageView+Coloring.h"

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
  self.icon.mwm_coloring = isEnabled ? MWMImageColoringBlack : MWMImageColoringGray;
  self.label.textColor = isEnabled ? [UIColor blackPrimaryText] : [UIColor blackHintText];
  self.promo = NO;
}

- (void)configurePromoWithImageName:(NSString *)imageName
                              label:(NSString *)label
{
  self.icon.image = [UIImage imageNamed:imageName];
  self.label.text = label;
  self.icon.mwm_coloring = MWMImageColoringBlue;
  self.label.textColor = [UIColor linkBlue];
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
    self.label.textColor = highlighted ? [UIColor linkBlueHighlighted] : [UIColor linkBlue];
  else
    self.label.textColor = highlighted ? [UIColor blackHintText] : [UIColor blackPrimaryText];
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
