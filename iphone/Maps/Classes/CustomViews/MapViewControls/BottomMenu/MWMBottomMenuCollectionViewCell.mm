#import "MWMBottomMenuCollectionViewCell.h"
#import "UIFont+MapsMeFonts.h"
#import "UIColor+MapsMeColor.h"

@interface MWMBottomMenuCollectionViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * label;
@property (weak, nonatomic) IBOutlet UIView * badgeBackground;
@property (weak, nonatomic) IBOutlet UILabel * badgeCount;
@property (weak, nonatomic) IBOutlet UIView * separator;

@property (nonatomic) BOOL isWideMenu;

@end

@implementation MWMBottomMenuCollectionViewCell

- (void)configureWithImage:(UIImage *)image
                     label:(NSString *)label
                badgeCount:(NSUInteger)badgeCount
{
  self.icon.image = image;
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
  [self highlighted:NO];
}

- (void)configureWithImageName:(NSString *)imageName label:(NSString *)label badgeCount:(NSUInteger)badgeCount
{
  [self configureWithImage:[UIImage imageNamed:imageName] label:label badgeCount:badgeCount];
}

- (void)highlighted:(BOOL)highlighted
{
  self.icon.tintColor = self.label.textColor = highlighted ? [UIColor blackHintText] : [UIColor blackSecondaryText];
}

@end
