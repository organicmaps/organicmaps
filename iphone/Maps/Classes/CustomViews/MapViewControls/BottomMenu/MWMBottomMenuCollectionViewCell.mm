#import "MWMBottomMenuCollectionViewCell.h"
#import "UIFont+MapsMeFonts.h"
#import "UIColor+MapsMeColor.h"

@interface MWMBottomMenuCollectionViewCell ()

@property(weak, nonatomic) IBOutlet UILabel * label;
@property(weak, nonatomic) IBOutlet UIView * badgeBackground;
@property(weak, nonatomic) IBOutlet UILabel * badgeCount;
@property(weak, nonatomic) IBOutlet UIView * separator;

@property(nonatomic) BOOL isWideMenu;

@property(copy, nonatomic) NSString * iconName;

@end

@implementation MWMBottomMenuCollectionViewCell

- (void)configureWithIconName:(NSString *)iconName
                        label:(NSString *)label
                   badgeCount:(NSUInteger)badgeCount
{
  self.iconName = iconName;
  self.icon.image = [UIImage imageNamed:iconName];
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

- (void)highlighted:(BOOL)highlighted
{
  NSString * pressed = highlighted ? @"_press" : @"";
  self.icon.image = [UIImage imageNamed:[self.iconName stringByAppendingString:pressed]];
  self.label.textColor = highlighted ? [UIColor blackHintText] : [UIColor blackPrimaryText];
}

@end
