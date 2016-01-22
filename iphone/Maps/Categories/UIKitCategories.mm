#import "Common.h"
#import "UIButton+Coloring.h"
#import "UIColor+MapsMeColor.h"
#import "UIImageView+Coloring.h"
#import "UIKitCategories.h"

@implementation NSObject (Optimized)

+ (NSString *)className
{
  return NSStringFromClass(self);
}

- (void)performAfterDelay:(NSTimeInterval)delayInSec block:(TMWMVoidBlock)block
{
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSec * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
    block();
  });
}

@end

@implementation UIView (Coordinates)

- (void)setMidX:(CGFloat)midX
{
  self.center = CGPointMake(midX, self.center.y);
}

- (CGFloat)midX
{
  return self.center.x;
}

- (void)setMidY:(CGFloat)midY
{
  self.center = CGPointMake(self.center.x, midY);
}

- (CGFloat)midY
{
  return self.center.y;
}

- (void)setOrigin:(CGPoint)origin
{
  self.frame = CGRectMake(origin.x, origin.y, self.frame.size.width, self.frame.size.height);
}

- (CGPoint)origin
{
  return self.frame.origin;
}

- (void)setMinX:(CGFloat)minX
{
  self.frame = CGRectMake(minX, self.frame.origin.y, self.frame.size.width, self.frame.size.height);
}

- (CGFloat)minX
{
  return self.frame.origin.x;
}

- (void)setMinY:(CGFloat)minY
{
  self.frame = CGRectMake(self.frame.origin.x, minY, self.frame.size.width, self.frame.size.height);
}

- (CGFloat)minY
{
  return self.frame.origin.y;
}

- (void)setMaxX:(CGFloat)maxX
{
  self.frame = CGRectMake(maxX - self.frame.size.width, self.frame.origin.y, self.frame.size.width, self.frame.size.height);
}

- (CGFloat)maxX
{
  return self.frame.origin.x + self.frame.size.width;
}

- (void)setMaxY:(CGFloat)maxY
{
  self.frame = CGRectMake(self.frame.origin.x, maxY - self.frame.size.height, self.frame.size.width, self.frame.size.height);
}

- (CGFloat)maxY
{
  return self.frame.origin.y + self.frame.size.height;
}

- (void)setWidth:(CGFloat)width
{
  self.frame = CGRectMake(self.frame.origin.x, self.frame.origin.y, width, self.frame.size.height);
}

- (CGFloat)width
{
  return self.frame.size.width;
}

- (void)setHeight:(CGFloat)height
{
  self.frame = CGRectMake(self.frame.origin.x, self.frame.origin.y, self.frame.size.width, height);
}

- (CGFloat)height
{
  return self.frame.size.height;
}

- (CGSize)size
{
  return self.frame.size;
}

- (void)setSize:(CGSize)size
{
  self.frame = CGRectMake(self.frame.origin.x, self.frame.origin.y, size.width, size.height);
}

+ (void)animateWithDuration:(NSTimeInterval)duration delay:(NSTimeInterval)delay damping:(double)dampingRatio initialVelocity:(double)springVelocity options:(UIViewAnimationOptions)options animations:(TMWMVoidBlock)animations completion:(void (^)(BOOL))completion
{
  [UIView animateWithDuration:duration delay:delay usingSpringWithDamping:dampingRatio initialSpringVelocity:springVelocity options:options animations:animations completion:completion];
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
  NSString * urlString = isIOSVersionLessThan(8) ?
  [NSString stringWithFormat:@"itms-apps://itunes.apple.com/app/id510623322?mt=8&at=1l3v7ya&ct=%@", launchPlaceName] :
  @"itms-apps://itunes.apple.com/WebObjects/MZStore.woa/wa/viewContentsUserReviews?id=510623322&onlyLatestVersion=true&pageNumber=0&sortOrdering=1&type=Purple+Software";
  [self openURL:[NSURL URLWithString:urlString]];
}

@end

@implementation NSString (Size)

- (CGSize)sizeWithDrawSize:(CGSize)drawSize font:(UIFont *)font
{
  CGRect rect = [self boundingRectWithSize:drawSize options:NSStringDrawingUsesLineFragmentOrigin attributes:@{NSFontAttributeName : font} context:nil];
  return CGRectIntegral(rect).size;
}

@end

@implementation SolidTouchView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {}

@end

@implementation UIView (Refresh)

- (void)refresh
{
  UIColor * opposite = self.backgroundColor.opposite;
  if (opposite)
    self.backgroundColor = opposite;

  for (UIView * v in self.subviews)
  {
    if ([v respondsToSelector:@selector(refresh)])
      [v refresh];
  }
  [self setNeedsDisplay];
}

@end

@implementation UITableViewCell (Refresh)

- (void)refresh
{
  if (isIOSVersionLessThan(8))
  {
    UIColor * opposite = self.backgroundColor.opposite;
    if (opposite)
      self.backgroundColor = opposite;

      for (UIView * v in self.subviews)
      {
        // There is workaroung for iOS7 only.
        if ([v isKindOfClass:NSClassFromString(@"UITableViewCellScrollView")])
        {
          for (UIView * subview in v.subviews)
           [subview refresh];
        }

        if ([v respondsToSelector:@selector(refresh)])
          [v refresh];
      }
  }
  else
  {
    [super refresh];
  }
  [self.selectedBackgroundView refresh];
}

@end

@implementation UINavigationBar (Refresh)

- (void)refresh
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

- (void)refresh
{
  [super refresh];
  UIColor * oppositeText = self.textColor.opposite;
  if (oppositeText)
    self.textColor = oppositeText;
}

@end

@implementation UISlider (Refresh)

- (void)refresh
{
  UIColor * opposite = self.minimumTrackTintColor.opposite;
  if (opposite)
    self.minimumTrackTintColor = opposite;
}

@end

@implementation UISwitch (Refresh)

- (void)refresh
{
  UIColor * opposite = self.onTintColor.opposite;
  if (opposite)
    self.onTintColor = opposite;
}

@end

@implementation UIButton (Refresh)

- (void)refresh
{
  [self changeColoringToOpposite];
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
}

@end

@implementation UITextView (Refresh)

- (void)refresh
{
  [super refresh];
  UIColor * oppositeText = self.textColor.opposite;
  UIColor * oppositeTint = self.tintColor.opposite;
  if (oppositeText)
    self.textColor = oppositeText;
  if (oppositeTint)
    self.tintColor = oppositeTint;
}

@end

@implementation UITextField (Refresh)

- (void)refresh
{
  [super refresh];
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

- (void)refresh
{
  [super refresh];
  [self changeColoringToOpposite];
}

@end

@implementation UIImageView (IOS7Workaround)

- (void)makeImageAlwaysTemplate
{
  if (isIOSVersionLessThan(8))
    self.image = [self.image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
}

@end

@implementation SolidTouchImageView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {}

@end

#import <objc/runtime.h>

static const void * UIAlertViewOriginalDelegateKey                   = & UIAlertViewOriginalDelegateKey;

static const void * UIAlertViewTapBlockKey                           = & UIAlertViewTapBlockKey;
static const void * UIAlertViewWillPresentBlockKey                   = & UIAlertViewWillPresentBlockKey;
static const void * UIAlertViewDidPresentBlockKey                    = & UIAlertViewDidPresentBlockKey;
static const void * UIAlertViewWillDismissBlockKey                   = & UIAlertViewWillDismissBlockKey;
static const void * UIAlertViewDidDismissBlockKey                    = & UIAlertViewDidDismissBlockKey;
static const void * UIAlertViewCancelBlockKey                        = & UIAlertViewCancelBlockKey;
static const void * UIAlertViewShouldEnableFirstOtherButtonBlockKey  = & UIAlertViewShouldEnableFirstOtherButtonBlockKey;

@implementation UIAlertView (Blocks)

- (void)_checkAlertViewDelegate
{
  if (self.delegate != (id<UIAlertViewDelegate>)self)
  {
    objc_setAssociatedObject(self, UIAlertViewOriginalDelegateKey, self.delegate, OBJC_ASSOCIATION_ASSIGN);
    self.delegate = (id <UIAlertViewDelegate>)self;
  }
}

- (MWMAlertViewCompletionBlock)tapBlock
{
  return objc_getAssociatedObject(self, UIAlertViewTapBlockKey);
}

- (void)setTapBlock:(MWMAlertViewCompletionBlock)tapBlock
{
  [self _checkAlertViewDelegate];
  objc_setAssociatedObject(self, UIAlertViewTapBlockKey, tapBlock, OBJC_ASSOCIATION_COPY);
}

- (MWMAlertViewCompletionBlock)willDismissBlock
{
  return objc_getAssociatedObject(self, UIAlertViewWillDismissBlockKey);
}

- (void)setWillDismissBlock:(MWMAlertViewCompletionBlock)willDismissBlock
{
  [self _checkAlertViewDelegate];
  objc_setAssociatedObject(self, UIAlertViewWillDismissBlockKey, willDismissBlock, OBJC_ASSOCIATION_COPY);
}

- (MWMAlertViewCompletionBlock)didDismissBlock
{
  return objc_getAssociatedObject(self, UIAlertViewDidDismissBlockKey);
}

- (void)setDidDismissBlock:(MWMAlertViewCompletionBlock)didDismissBlock
{
  [self _checkAlertViewDelegate];
  objc_setAssociatedObject(self, UIAlertViewDidDismissBlockKey, didDismissBlock, OBJC_ASSOCIATION_COPY);
}

- (MWMAlertViewBlock)willPresentBlock
{
  return objc_getAssociatedObject(self, UIAlertViewWillPresentBlockKey);
}

- (void)setWillPresentBlock:(MWMAlertViewBlock)willPresentBlock
{
  [self _checkAlertViewDelegate];
  objc_setAssociatedObject(self, UIAlertViewWillPresentBlockKey, willPresentBlock, OBJC_ASSOCIATION_COPY);
}

- (MWMAlertViewBlock)didPresentBlock
{
  return objc_getAssociatedObject(self, UIAlertViewDidPresentBlockKey);
}

- (void)setDidPresentBlock:(MWMAlertViewBlock)didPresentBlock
{
  [self _checkAlertViewDelegate];
  objc_setAssociatedObject(self, UIAlertViewDidPresentBlockKey, didPresentBlock, OBJC_ASSOCIATION_COPY);
}

- (MWMAlertViewBlock)cancelBlock
{
  return objc_getAssociatedObject(self, UIAlertViewCancelBlockKey);
}

- (void)setCancelBlock:(MWMAlertViewBlock)cancelBlock
{
  [self _checkAlertViewDelegate];
  objc_setAssociatedObject(self, UIAlertViewCancelBlockKey, cancelBlock, OBJC_ASSOCIATION_COPY);
}

- (void)setShouldEnableFirstOtherButtonBlock:(BOOL(^)(UIAlertView * alertView))shouldEnableFirstOtherButtonBlock
{
  [self _checkAlertViewDelegate];
  objc_setAssociatedObject(self, UIAlertViewShouldEnableFirstOtherButtonBlockKey, shouldEnableFirstOtherButtonBlock, OBJC_ASSOCIATION_COPY);
}

- (BOOL(^)(UIAlertView * alertView))shouldEnableFirstOtherButtonBlock
{
  return objc_getAssociatedObject(self, UIAlertViewShouldEnableFirstOtherButtonBlockKey);
}

#pragma mark - UIAlertViewDelegate

- (void)willPresentAlertView:(UIAlertView *)alertView
{
  MWMAlertViewBlock block = alertView.willPresentBlock;
  
  if (block)
    block(alertView);
  
  id originalDelegate = objc_getAssociatedObject(self, UIAlertViewOriginalDelegateKey);
  if (originalDelegate && [originalDelegate respondsToSelector:@selector(willPresentAlertView:)])
    [originalDelegate willPresentAlertView:alertView];
}

- (void)didPresentAlertView:(UIAlertView *)alertView
{
  MWMAlertViewBlock block = alertView.didPresentBlock;
  
  if (block)
    block(alertView);
  
  id originalDelegate = objc_getAssociatedObject(self, UIAlertViewOriginalDelegateKey);
  if (originalDelegate && [originalDelegate respondsToSelector:@selector(didPresentAlertView:)])
    [originalDelegate didPresentAlertView:alertView];
}

- (void)alertViewCancel:(UIAlertView *)alertView {
  MWMAlertViewBlock block = alertView.cancelBlock;
  
  if (block)
    block(alertView);
  
  id originalDelegate = objc_getAssociatedObject(self, UIAlertViewOriginalDelegateKey);
  if (originalDelegate && [originalDelegate respondsToSelector:@selector(alertViewCancel:)])
    [originalDelegate alertViewCancel:alertView];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
  MWMAlertViewCompletionBlock completion = alertView.tapBlock;
  
  if (completion)
    completion(alertView, buttonIndex);
  
  id originalDelegate = objc_getAssociatedObject(self, UIAlertViewOriginalDelegateKey);
  if (originalDelegate && [originalDelegate respondsToSelector:@selector(alertView:clickedButtonAtIndex:)])
    [originalDelegate alertView:alertView clickedButtonAtIndex:buttonIndex];
}

- (void)alertView:(UIAlertView *)alertView willDismissWithButtonIndex:(NSInteger)buttonIndex {
  MWMAlertViewCompletionBlock completion = alertView.willDismissBlock;
  
  if (completion)
    completion(alertView, buttonIndex);
  
  id originalDelegate = objc_getAssociatedObject(self, UIAlertViewOriginalDelegateKey);
  if (originalDelegate && [originalDelegate respondsToSelector:@selector(alertView:willDismissWithButtonIndex:)])
    [originalDelegate alertView:alertView willDismissWithButtonIndex:buttonIndex];
}

- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
{
  MWMAlertViewCompletionBlock completion = alertView.didDismissBlock;
  
  if (completion)
    completion(alertView, buttonIndex);
  
  id originalDelegate = objc_getAssociatedObject(self, UIAlertViewOriginalDelegateKey);
  if (originalDelegate && [originalDelegate respondsToSelector:@selector(alertView:didDismissWithButtonIndex:)])
    [originalDelegate alertView:alertView didDismissWithButtonIndex:buttonIndex];
}

- (BOOL)alertViewShouldEnableFirstOtherButton:(UIAlertView *)alertView
{
  BOOL(^shouldEnableFirstOtherButtonBlock)(UIAlertView * alertView) = alertView.shouldEnableFirstOtherButtonBlock;
  
  if (shouldEnableFirstOtherButtonBlock)
    return shouldEnableFirstOtherButtonBlock(alertView);
  
  id originalDelegate = objc_getAssociatedObject(self, UIAlertViewOriginalDelegateKey);
  if (originalDelegate && [originalDelegate respondsToSelector:@selector(alertViewShouldEnableFirstOtherButton:)])
    return [originalDelegate alertViewShouldEnableFirstOtherButton:alertView];
  
  return YES;
}

@end

@implementation UINavigationController (Autorotate)

- (BOOL)shouldAutorotate
{
  return [[self.viewControllers lastObject] shouldAutorotate];
}

- (NSUInteger)supportedInterfaceOrientations
{
  return UIInterfaceOrientationMaskAll;
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