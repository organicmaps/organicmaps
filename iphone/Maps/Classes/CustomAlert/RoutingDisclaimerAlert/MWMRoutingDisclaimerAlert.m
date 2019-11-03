#import "MWMRoutingDisclaimerAlert.h"
#import "MWMAlertViewController.h"
#import "Statistics.h"

static CGFloat const kMinimumOffset = 20.;
static NSString * const kStatisticsEvent = @"Routing Disclaimer Alert";

@interface MWMRoutingDisclaimerAlert ()

@property(weak, nonatomic) IBOutlet UITextView * textView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * textViewHeight;
@property(copy, nonatomic) MWMVoidBlock okBlock;

@end

@implementation MWMRoutingDisclaimerAlert

+ (instancetype)alertWithOkBlock:(MWMVoidBlock)block
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatOpen}];
  MWMRoutingDisclaimerAlert * alert =
      [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  NSString * message = [NSString stringWithFormat:@"%@\n\n%@\n\n%@\n\n%@\n\n%@",
                                                  L(@"dialog_routing_disclaimer_priority"),
                                                  L(@"dialog_routing_disclaimer_precision"),
                                                  L(@"dialog_routing_disclaimer_recommendations"),
                                                  L(@"dialog_routing_disclaimer_borders"),
                                                  L(@"dialog_routing_disclaimer_beware")];

  alert.textView.attributedText =
      [[NSAttributedString alloc] initWithString:message
                                      attributes:@{
                                        NSFontAttributeName : UIFont.regular14,
                                        NSForegroundColorAttributeName : UIColor.blackSecondaryText
                                      }];
  [alert.textView sizeToFit];
  UIWindow * window = UIApplication.sharedApplication.keyWindow;
  [alert invalidateTextViewHeight:alert.textView.height withHeight:window.height];
  alert.okBlock = block;
  return alert;
}

- (IBAction)okTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatApply}];
  [self close:self.okBlock];
}

- (IBAction)cancelTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatCancel}];
  [self close:nil];
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  UIView * superview = self.superview ?: UIApplication.sharedApplication.keyWindow;
  CGFloat const height = UIInterfaceOrientationIsLandscape(orientation)
                             ? MIN(superview.width, superview.height)
                             : MAX(superview.width, superview.height);
  [self invalidateTextViewHeight:self.textView.contentSize.height withHeight:height];
}

- (void)invalidateTextViewHeight:(CGFloat)textViewHeight withHeight:(CGFloat)height
{
  self.textViewHeight.constant = [self bounded:textViewHeight withHeight:height];
  self.textView.scrollEnabled = textViewHeight > self.textViewHeight.constant;
  [self layoutIfNeeded];
}

- (CGFloat)bounded:(CGFloat)f withHeight:(CGFloat)h
{
  CGFloat const currentHeight = [self.subviews.firstObject height];
  CGFloat const maximumHeight = h - 2. * kMinimumOffset;
  CGFloat const availableHeight = maximumHeight - currentHeight;
  return MIN(f, availableHeight + self.textViewHeight.constant);
}

@end
