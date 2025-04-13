#import "MWMAlert.h"

typedef NS_ENUM(NSInteger, MWMMobileInternetAlertResult) {
  MWMMobileInternetAlertResultAlways,
  MWMMobileInternetAlertResultToday,
  MWMMobileInternetAlertResultNotToday
};

NS_ASSUME_NONNULL_BEGIN

typedef void(^MWMMobileInternetAlertCompletionBlock)(MWMMobileInternetAlertResult result);

@interface MWMMobileInternetAlert : MWMAlert

+ (instancetype)alertWithBlock:(MWMMobileInternetAlertCompletionBlock)block;

@end

NS_ASSUME_NONNULL_END
