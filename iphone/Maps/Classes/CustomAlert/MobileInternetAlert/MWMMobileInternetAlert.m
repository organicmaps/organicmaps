#import "MWMMobileInternetAlert.h"

@interface MWMMobileInternetAlert ()

@property(copy, nonatomic) MWMMobileInternetAlertCompletionBlock completionBlock;

@end

@implementation MWMMobileInternetAlert

+ (instancetype)alertWithBlock:(MWMMobileInternetAlertCompletionBlock)block;
{
  MWMMobileInternetAlert * alert =
      [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  alert.completionBlock = block;
  return alert;
}

- (IBAction)alwaysTap
{
  [self close:^{
    self.completionBlock(MWMMobileInternetAlertResultAlways);
  }];
}

- (IBAction)askTap
{
  [self close:^{
    self.completionBlock(MWMMobileInternetAlertResultToday);
  }];
}

- (IBAction)neverTap
{
  [self close:^{
    self.completionBlock(MWMMobileInternetAlertResultNotToday);
  }];
}

@end
