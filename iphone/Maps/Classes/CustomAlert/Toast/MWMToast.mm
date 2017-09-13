#import "MWMToast.h"
#import "MWMCommon.h"
#import "SwiftBridge.h"

namespace
{
void * kContext = &kContext;
NSString * const kKeyPath = @"sublayers";
NSUInteger const kWordsPerSecond = 3;
}  // namespace

@interface MWMToast ()

@property(nonatomic) IBOutlet UIView * rootView;
@property(nonatomic) IBOutlet UILabel * label;

@property(nonatomic) NSLayoutConstraint * bottomOffset;

@end

@implementation MWMToast

+ (void)showWithText:(NSString *)text
{
  MWMToast * toast = [self toast];
  toast.label.text = text;
  toast.label.textColor = [UIColor blackPrimaryText];
  toast.rootView.backgroundColor = [UIColor toastBackground];
  [toast show];
}

+ (BOOL)affectsStatusBar { return [self toast].rootView.superview != nil; }
+ (UIStatusBarStyle)preferredStatusBarStyle
{
  setStatusBarBackgroundColor(UIColor.clearColor);
  return [UIColor isNightMode] ? UIStatusBarStyleLightContent : UIStatusBarStyleDefault;
}

+ (MWMToast *)toast
{
  static MWMToast * toast;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    toast = [[super alloc] initToast];
  });
  return toast;
}

- (instancetype)initToast
{
  self = [super init];
  if (self)
  {
    [NSBundle.mainBundle loadNibNamed:[[self class] className] owner:self options:nil];
    self.rootView.translatesAutoresizingMaskIntoConstraints = NO;
  }
  return self;
}

- (void)configLayout
{
  UIView * sv = self.rootView;
  [sv removeFromSuperview];

  auto tvc = [UIViewController topViewController];
  UIView * ov = tvc.view;
  if (!tvc.navigationController.navigationBarHidden)
    ov = tvc.navigationController.navigationBar.superview;
  [ov addSubview:sv];

  NSLayoutConstraint * topOffset = [NSLayoutConstraint constraintWithItem:sv
                                                                attribute:NSLayoutAttributeTop
                                                                relatedBy:NSLayoutRelationEqual
                                                                   toItem:ov
                                                                attribute:NSLayoutAttributeTop
                                                               multiplier:1
                                                                 constant:0];
  topOffset.priority = UILayoutPriorityDefaultLow;
  self.bottomOffset = [NSLayoutConstraint constraintWithItem:sv
                                                   attribute:NSLayoutAttributeBottom
                                                   relatedBy:NSLayoutRelationEqual
                                                      toItem:ov
                                                   attribute:NSLayoutAttributeTop
                                                  multiplier:1
                                                    constant:0];
  self.bottomOffset.priority = UILayoutPriorityDefaultHigh;
  NSLayoutConstraint * leadingOffset =
      [NSLayoutConstraint constraintWithItem:sv
                                   attribute:NSLayoutAttributeLeading
                                   relatedBy:NSLayoutRelationEqual
                                      toItem:ov
                                   attribute:NSLayoutAttributeLeading
                                  multiplier:1
                                    constant:0];
  NSLayoutConstraint * trailingOffset =
      [NSLayoutConstraint constraintWithItem:sv
                                   attribute:NSLayoutAttributeTrailing
                                   relatedBy:NSLayoutRelationEqual
                                      toItem:ov
                                   attribute:NSLayoutAttributeTrailing
                                  multiplier:1
                                    constant:0];
  [ov addConstraints:@[ topOffset, self.bottomOffset, leadingOffset, trailingOffset ]];
  [ov setNeedsLayout];
}

- (void)scheduleHide
{
  [NSObject cancelPreviousPerformRequestsWithTarget:self];
  NSString * text = self.label.text;
  NSArray<NSString *> * words = [text componentsSeparatedByString:@" "];
  NSTimeInterval const delay = MAX(2, 1 + words.count / kWordsPerSecond);
  [self performSelector:@selector(hide) withObject:nil afterDelay:delay];
}

- (void)show
{
  [self configLayout];

  UIView * ov = [UIViewController topViewController].view;
  dispatch_async(dispatch_get_main_queue(), ^{
    [ov layoutIfNeeded];
    self.bottomOffset.priority = UILayoutPriorityFittingSizeLevel;
    [UIView animateWithDuration:kDefaultAnimationDuration
        animations:^{
          [ov layoutIfNeeded];
        }
        completion:^(BOOL finished) {
          [self subscribe];
          [[UIViewController topViewController] setNeedsStatusBarAppearanceUpdate];
          [self scheduleHide];
        }];
  });
}

- (void)hide
{
  [self unsubscribe];

  UIView * ov = [UIViewController topViewController].view;
  [ov layoutIfNeeded];
  self.bottomOffset.priority = UILayoutPriorityDefaultHigh;
  [UIView animateWithDuration:kDefaultAnimationDuration
      animations:^{
        [ov layoutIfNeeded];
      }
      completion:^(BOOL finished) {
        [self.rootView removeFromSuperview];
        [[UIViewController topViewController] setNeedsStatusBarAppearanceUpdate];
      }];
}

- (void)subscribe
{
  UIView * sv = self.rootView;
  UIView * ov = sv.superview;
  CALayer * ol = ov.layer;
  [ol addObserver:self forKeyPath:kKeyPath options:NSKeyValueObservingOptionNew context:kContext];
}

- (void)unsubscribe
{
  UIView * sv = self.rootView;
  UIView * ov = sv.superview;
  CALayer * ol = ov.layer;
  [ol removeObserver:self forKeyPath:kKeyPath context:kContext];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
  if (context == kContext)
  {
    UIView * sv = self.rootView;
    UIView * ov = sv.superview;
    [ov bringSubviewToFront:sv];
    return;
  }
  [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
}
@end
