#import "UIKitCategories.h"
#import "MWMCommon.h"
#import "UIButton+RuntimeAttributes.h"
#import "UIImageView+Coloring.h"

#import <Crashlytics/Crashlytics.h>
#import <SafariServices/SafariServices.h>

@implementation NSObject (Optimized)

+ (NSString *)className { return NSStringFromClass(self); }
- (void)performAfterDelay:(NSTimeInterval)delayInSec block:(MWMVoidBlock)block
{
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSec * NSEC_PER_SEC)),
                 dispatch_get_main_queue(), ^{
                   block();
                 });
}

@end

@implementation UIView (Coordinates)

- (void)setMidX:(CGFloat)midX { self.center = CGPointMake(midX, self.center.y); }
- (CGFloat)midX { return self.center.x; }
- (void)setMidY:(CGFloat)midY { self.center = CGPointMake(self.center.x, midY); }
- (CGFloat)midY { return self.center.y; }
- (void)setOrigin:(CGPoint)origin
{
  self.frame = CGRectMake(origin.x, origin.y, self.frame.size.width, self.frame.size.height);
}

- (CGPoint)origin { return self.frame.origin; }
- (void)setMinX:(CGFloat)minX
{
  self.frame = CGRectMake(minX, self.frame.origin.y, self.frame.size.width, self.frame.size.height);
}

- (CGFloat)minX { return self.frame.origin.x; }
- (void)setMinY:(CGFloat)minY
{
  self.frame = CGRectMake(self.frame.origin.x, minY, self.frame.size.width, self.frame.size.height);
}

- (CGFloat)minY { return self.frame.origin.y; }
- (void)setMaxX:(CGFloat)maxX
{
  self.frame = CGRectMake(maxX - self.frame.size.width, self.frame.origin.y, self.frame.size.width,
                          self.frame.size.height);
}

- (CGFloat)maxX { return self.frame.origin.x + self.frame.size.width; }
- (void)setMaxY:(CGFloat)maxY
{
  self.frame = CGRectMake(self.frame.origin.x, maxY - self.frame.size.height, self.frame.size.width,
                          self.frame.size.height);
}

- (CGFloat)maxY { return self.frame.origin.y + self.frame.size.height; }
- (void)setWidth:(CGFloat)width
{
  self.frame = CGRectMake(self.frame.origin.x, self.frame.origin.y, width, self.frame.size.height);
}

- (CGFloat)width { return self.frame.size.width; }
- (void)setHeight:(CGFloat)height
{
  self.frame = CGRectMake(self.frame.origin.x, self.frame.origin.y, self.frame.size.width, height);
}

- (CGFloat)height { return self.frame.size.height; }
- (CGSize)size { return self.frame.size; }
- (void)setSize:(CGSize)size
{
  self.frame = CGRectMake(self.frame.origin.x, self.frame.origin.y, size.width, size.height);
}

+ (void)animateWithDuration:(NSTimeInterval)duration
                      delay:(NSTimeInterval)delay
                    damping:(double)dampingRatio
            initialVelocity:(double)springVelocity
                    options:(UIViewAnimationOptions)options
                 animations:(MWMVoidBlock)animations
                 completion:(void (^)(BOOL))completion
{
  [UIView animateWithDuration:duration
                        delay:delay
       usingSpringWithDamping:dampingRatio
        initialSpringVelocity:springVelocity
                      options:options
                   animations:animations
                   completion:completion];
}

- (void)sizeToIntegralFit
{
  [self sizeToFit];
  self.frame = CGRectIntegral(self.frame);
}

@end

@implementation UIApplication (URLs)

- (void)rateVersionFrom:(NSString *)launchPlaceName
{
  NSString * urlString =
      @"itms-apps://itunes.apple.com/WebObjects/MZStore.woa/wa/"
      @"viewContentsUserReviews?id=510623322&onlyLatestVersion=true&pageNumber=0&"
      @"sortOrdering=1&type=Purple+Software";
  NSURL * url = [NSURL URLWithString:urlString];
  [self openURL:url];
}

@end

@implementation SolidTouchView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {}
@end

@implementation UIView (Refresh)

- (void)mwm_refreshUI
{
  UIColor * opposite = self.backgroundColor.opposite;
  if (opposite)
    self.backgroundColor = opposite;

  for (UIView * v in self.subviews)
  {
    if ([v respondsToSelector:@selector(mwm_refreshUI)])
      [v mwm_refreshUI];
  }
  [self setNeedsDisplay];
}

@end

@implementation UITableViewCell (Refresh)

- (void)mwm_refreshUI
{
  [super mwm_refreshUI];
  [self.selectedBackgroundView mwm_refreshUI];
}

@end

@implementation UINavigationBar (Refresh)

- (void)mwm_refreshUI
{
  UIColor * oppositeTint = self.tintColor.opposite;
  UIColor * oppositeBar = self.barTintColor.opposite;
  if (oppositeTint)
    self.tintColor = oppositeTint;
  if (oppositeBar)
    self.barTintColor = oppositeBar;
}

@end

@implementation UILabel (Refresh)

- (void)mwm_refreshUI
{
  [super mwm_refreshUI];
  UIColor * oppositeText = self.textColor.opposite;
  if (oppositeText)
    self.textColor = oppositeText;
}

@end

@implementation UISlider (Refresh)

- (void)mwm_refreshUI
{
  UIColor * opposite = self.minimumTrackTintColor.opposite;
  if (opposite)
    self.minimumTrackTintColor = opposite;
}

@end

@implementation UISwitch (Refresh)

- (void)mwm_refreshUI
{
  UIColor * opposite = self.onTintColor.opposite;
  if (opposite)
    self.onTintColor = opposite;
}

@end

@implementation UIButton (Refresh)

- (void)mwm_refreshUI
{
  UIColor * oppositeNormal = [self titleColorForState:UIControlStateNormal].opposite;
  UIColor * oppositeSelected = [self titleColorForState:UIControlStateSelected].opposite;
  UIColor * oppositeHightlighted = [self titleColorForState:UIControlStateHighlighted].opposite;
  UIColor * oppositeDisabled = [self titleColorForState:UIControlStateDisabled].opposite;
  if (oppositeNormal)
    [self setTitleColor:oppositeNormal forState:UIControlStateNormal];
  if (oppositeSelected)
    [self setTitleColor:oppositeSelected forState:UIControlStateSelected];
  if (oppositeHightlighted)
    [self setTitleColor:oppositeHightlighted forState:UIControlStateHighlighted];
  if (oppositeDisabled)
    [self setTitleColor:oppositeDisabled forState:UIControlStateDisabled];

  NSString * backgroundColorName = [self backgroundColorName];
  NSString * backgroundHighlightedColorName = [self backgroundHighlightedColorName];
  NSString * backgroundSelectedColorName = [self backgroundSelectedColorName];
  if (backgroundColorName)
    [self setBackgroundColorName:backgroundColorName];
  if (backgroundHighlightedColorName)
    [self setBackgroundHighlightedColorName:backgroundHighlightedColorName];
  if (backgroundSelectedColorName)
    [self setBackgroundSelectedColorName:backgroundSelectedColorName];
}

@end

@implementation UITextView (Refresh)

- (void)mwm_refreshUI
{
  [super mwm_refreshUI];
  UIColor * oppositeText = self.textColor.opposite;
  UIColor * oppositeTint = self.tintColor.opposite;
  if (oppositeText)
    self.textColor = oppositeText;
  if (oppositeTint)
    self.tintColor = oppositeTint;
}

@end

@implementation UITextField (Refresh)

- (void)mwm_refreshUI
{
  [super mwm_refreshUI];
  UIColor * oppositeText = self.textColor.opposite;
  UILabel * placeholder = [self valueForKey:@"_placeholderLabel"];
  UIColor * oppositePlaceholder = placeholder.textColor.opposite;
  if (oppositeText)
    self.textColor = oppositeText;
  if (oppositePlaceholder)
    placeholder.textColor = oppositePlaceholder;
}

@end

@implementation UIImageView (Refresh)

- (void)mwm_refreshUI
{
  [super mwm_refreshUI];
  [self changeColoringToOpposite];
}

@end

@implementation SolidTouchImageView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {}
@end

@implementation UINavigationController (Autorotate)

- (BOOL)shouldAutorotate { return [self.viewControllers.lastObject shouldAutorotate]; }
@end

@implementation UIViewController (Autorotate)

- (NSUInteger)supportedInterfaceOrientations { return UIInterfaceOrientationMaskAll; }
@end

@interface UIViewController (SafariDelegateImpl)<SFSafariViewControllerDelegate>

@end

@implementation UIViewController (SafariDelegateImpl)

- (void)safariViewControllerDidFinish:(SFSafariViewController *)controller
{
  [self.navigationController dismissViewControllerAnimated:YES completion:nil];
}

@end

@implementation UIViewController (Safari)

- (void)openUrl:(NSURL *)url
{
  if (!url)
  {
    NSAssert(false, @"URL is nil!");
    auto err = [[NSError alloc] initWithDomain:kMapsmeErrorDomain
                                          code:0
                                      userInfo:@{
                                        @"Trying to open nil url" : @YES
                                      }];
    [[Crashlytics sharedInstance] recordError:err];
    return;
  }
  NSString * scheme = url.scheme;
  if (!([scheme isEqualToString:@"http"] || [scheme isEqualToString:@"https"]))
  {
    NSAssert(false, @"Incorrect url's scheme!");
    auto urlString = url.absoluteString;
    auto err = [[NSError alloc] initWithDomain:kMapsmeErrorDomain
                                          code:0
                                      userInfo:@{
                                        @"Trying to open incorrect url" : urlString
                                      }];
    [[Crashlytics sharedInstance] recordError:err];
    return;
  }

  SFSafariViewController * svc = [[SFSafariViewController alloc] initWithURL:url];
  svc.delegate = self;
  [self.navigationController presentViewController:svc animated:YES completion:nil];
}

@end

@implementation UIImage (ImageWithColor)

+ (UIImage *)imageWithColor:(UIColor *)color
{
  CGRect rect = CGRectMake(0.0, 0.0, 1.0, 1.0);
  UIGraphicsBeginImageContext(rect.size);
  CGContextRef context = UIGraphicsGetCurrentContext();

  CGContextSetFillColorWithColor(context, color.CGColor);
  CGContextFillRect(context, rect);

  UIImage * image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();

  return image;
}

@end
