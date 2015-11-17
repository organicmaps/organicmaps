#import "MWMBottomMenuCollectionViewCell.h"
#import "UIFont+MapsMeFonts.h"
#import "UIColor+MapsMeColor.h"

@interface MWMBottomMenuCollectionViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * label;
@property (weak, nonatomic) IBOutlet UIView * badgeBackground;
@property (weak, nonatomic) IBOutlet UILabel * badgeCount;
@property (weak, nonatomic) IBOutlet UIView * separator;

@property (nonatomic) BOOL isWideMenu;

@property (nonatomic) UIImage * image;
@property (nonatomic) UIImage * highlightedImage;

@end

@implementation MWMBottomMenuCollectionViewCell

- (void)configureWithImage:(UIImage *)image
          highlightedImage:(UIImage *)highlightedImage
                     label:(NSString *)label
                badgeCount:(NSUInteger)badgeCount
{
  self.image = image;
  self.highlightedImage = highlightedImage;
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

- (void)configureWithImageName:(NSString *)imageName
                         label:(NSString *)label
                    badgeCount:(NSUInteger)badgeCount
{
  [self configureWithImage:[UIImage imageNamed:imageName]
          highlightedImage:[UIImage imageNamed:[imageName stringByAppendingString:@"_press"]]
                     label:label
                badgeCount:badgeCount];
}

- (void)highlighted:(BOOL)highlighted
{
  self.icon.image = highlighted ? self.highlightedImage : self.image;
  self.label.textColor = highlighted ? [UIColor blackHintText] : [UIColor blackPrimaryText];
}

@end
