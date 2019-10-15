#import "MWMMobileInternetAlert.h"
#import "Statistics.h"

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
  [Statistics logEvent:kStatMobileInternetAlert withParameters:@{kStatValue : kStatAlways}];
  [self close:^{
    self.completionBlock(MWMMobileInternetAlertResultAlways);
  }];
}

- (IBAction)askTap
{
  [Statistics logEvent:kStatMobileInternetAlert withParameters:@{kStatValue: kStatToday}];
  [self close:^{
    self.completionBlock(MWMMobileInternetAlertResultToday);
  }];
}

- (IBAction)neverTap
{
  [Statistics logEvent:kStatMobileInternetAlert withParameters:@{kStatAction: kStatNotToday}];
  [self close:^{
    self.completionBlock(MWMMobileInternetAlertResultNotToday);
  }];
}

@end
