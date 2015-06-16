
#import "UIKitCategories.h"

@implementation NSObject (Optimized)

+ (NSString *)className
{
  return NSStringFromClass(self);
}

- (void)performAfterDelay:(NSTimeInterval)delayInSec block:(void (^)(void))block
{
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSec * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
    block();
  });
}

@end


@implementation UIColor (HexColor)

+ (UIColor *)colorWithColorCode:(NSString *)hexString
{
  NSString * cleanString = [hexString stringByReplacingOccurrencesOfString:@"#" withString:@""];

  if (cleanString.length == 6)
    cleanString = [cleanString stringByAppendingString:@"ff"];

  unsigned int baseValue;
  [[NSScanner scannerWithString:cleanString] scanHexInt:&baseValue];

  float red = ((baseValue >> 24) & 0xFF) / 255.f;
  float green = ((baseValue >> 16) & 0xFF) / 255.f;
  float blue = ((baseValue >> 8) & 0xFF) / 255.f;
  float alpha = ((baseValue >> 0) & 0xFF) / 255.f;

  return [UIColor colorWithRed:red green:green blue:blue alpha:alpha];
}

+ (UIColor *)applicationBackgroundColor
{
  return [UIColor colorWithColorCode:@"efeff4"];
}

+ (UIColor *)applicationColor
{
  return [UIColor colorWithColorCode:@"15c783"];
}

+ (UIColor *)navigationBarColor
{
  return [UIColor colorWithColorCode:@"1F9952"];
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

+ (void)animateWithDuration:(NSTimeInterval)duration delay:(NSTimeInterval)delay damping:(double)dampingRatio initialVelocity:(double)springVelocity options:(UIViewAnimationOptions)options animations:(void (^)(void))animations completion:(void (^)(BOOL))completion
{
  if ([UIView respondsToSelector:@selector(animateWithDuration:delay:usingSpringWithDamping:initialSpringVelocity:options:animations:completion:)])
    [UIView animateWithDuration:duration delay:delay usingSpringWithDamping:dampingRatio initialSpringVelocity:springVelocity options:options animations:animations completion:completion];
  else
    [UIView animateWithDuration:(duration * dampingRatio) delay:delay options:options animations:animations completion:completion];
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
  NSString * urlString = [NSString stringWithFormat:@"itms-apps://itunes.apple.com/app/id510623322?mt=8&at=1l3v7ya&ct=%@", launchPlaceName];
  [self openURL:[NSURL URLWithString:urlString]];
}

@end


@implementation NSString (Size)

- (CGSize)sizeWithDrawSize:(CGSize)drawSize font:(UIFont *)font
{
  if ([self respondsToSelector:@selector(boundingRectWithSize:options:attributes:context:)])
  {
    CGRect rect = [self boundingRectWithSize:drawSize options:NSStringDrawingUsesLineFragmentOrigin attributes:@{NSFontAttributeName : font} context:nil];
    return CGRectIntegral(rect).size;
  }
  else
  {
    CGSize size = [self sizeWithFont:font constrainedToSize:drawSize lineBreakMode:NSLineBreakByWordWrapping];
    return CGRectIntegral(CGRectMake(0, 0, size.width, size.height)).size;
  }
}

@end


@implementation SolidTouchView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {}

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

- (UIAlertViewCompletionBlock)tapBlock
{
  return objc_getAssociatedObject(self, UIAlertViewTapBlockKey);
}

- (void)setTapBlock:(UIAlertViewCompletionBlock)tapBlock
{
  [self _checkAlertViewDelegate];
  objc_setAssociatedObject(self, UIAlertViewTapBlockKey, tapBlock, OBJC_ASSOCIATION_COPY);
}

- (UIAlertViewCompletionBlock)willDismissBlock
{
  return objc_getAssociatedObject(self, UIAlertViewWillDismissBlockKey);
}

- (void)setWillDismissBlock:(UIAlertViewCompletionBlock)willDismissBlock
{
  [self _checkAlertViewDelegate];
  objc_setAssociatedObject(self, UIAlertViewWillDismissBlockKey, willDismissBlock, OBJC_ASSOCIATION_COPY);
}

- (UIAlertViewCompletionBlock)didDismissBlock
{
  return objc_getAssociatedObject(self, UIAlertViewDidDismissBlockKey);
}

- (void)setDidDismissBlock:(UIAlertViewCompletionBlock)didDismissBlock
{
  [self _checkAlertViewDelegate];
  objc_setAssociatedObject(self, UIAlertViewDidDismissBlockKey, didDismissBlock, OBJC_ASSOCIATION_COPY);
}

- (UIAlertViewBlock)willPresentBlock
{
  return objc_getAssociatedObject(self, UIAlertViewWillPresentBlockKey);
}

- (void)setWillPresentBlock:(UIAlertViewBlock)willPresentBlock
{
  [self _checkAlertViewDelegate];
  objc_setAssociatedObject(self, UIAlertViewWillPresentBlockKey, willPresentBlock, OBJC_ASSOCIATION_COPY);
}

- (UIAlertViewBlock)didPresentBlock
{
  return objc_getAssociatedObject(self, UIAlertViewDidPresentBlockKey);
}

- (void)setDidPresentBlock:(UIAlertViewBlock)didPresentBlock
{
  [self _checkAlertViewDelegate];
  objc_setAssociatedObject(self, UIAlertViewDidPresentBlockKey, didPresentBlock, OBJC_ASSOCIATION_COPY);
}

- (UIAlertViewBlock)cancelBlock
{
  return objc_getAssociatedObject(self, UIAlertViewCancelBlockKey);
}

- (void)setCancelBlock:(UIAlertViewBlock)cancelBlock
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
  UIAlertViewBlock block = alertView.willPresentBlock;
  
  if (block)
    block(alertView);
  
  id originalDelegate = objc_getAssociatedObject(self, UIAlertViewOriginalDelegateKey);
  if (originalDelegate && [originalDelegate respondsToSelector:@selector(willPresentAlertView:)])
    [originalDelegate willPresentAlertView:alertView];
}

- (void)didPresentAlertView:(UIAlertView *)alertView
{
  UIAlertViewBlock block = alertView.didPresentBlock;
  
  if (block)
    block(alertView);
  
  id originalDelegate = objc_getAssociatedObject(self, UIAlertViewOriginalDelegateKey);
  if (originalDelegate && [originalDelegate respondsToSelector:@selector(didPresentAlertView:)])
    [originalDelegate didPresentAlertView:alertView];
}


- (void)alertViewCancel:(UIAlertView *)alertView {
  UIAlertViewBlock block = alertView.cancelBlock;
  
  if (block)
    block(alertView);
  
  id originalDelegate = objc_getAssociatedObject(self, UIAlertViewOriginalDelegateKey);
  if (originalDelegate && [originalDelegate respondsToSelector:@selector(alertViewCancel:)])
    [originalDelegate alertViewCancel:alertView];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
  UIAlertViewCompletionBlock completion = alertView.tapBlock;
  
  if (completion)
    completion(alertView, buttonIndex);
  
  id originalDelegate = objc_getAssociatedObject(self, UIAlertViewOriginalDelegateKey);
  if (originalDelegate && [originalDelegate respondsToSelector:@selector(alertView:clickedButtonAtIndex:)])
    [originalDelegate alertView:alertView clickedButtonAtIndex:buttonIndex];
}

- (void)alertView:(UIAlertView *)alertView willDismissWithButtonIndex:(NSInteger)buttonIndex {
  UIAlertViewCompletionBlock completion = alertView.willDismissBlock;
  
  if (completion)
    completion(alertView, buttonIndex);
  
  id originalDelegate = objc_getAssociatedObject(self, UIAlertViewOriginalDelegateKey);
  if (originalDelegate && [originalDelegate respondsToSelector:@selector(alertView:willDismissWithButtonIndex:)])
    [originalDelegate alertView:alertView willDismissWithButtonIndex:buttonIndex];
}

- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
{
  UIAlertViewCompletionBlock completion = alertView.didDismissBlock;
  
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
