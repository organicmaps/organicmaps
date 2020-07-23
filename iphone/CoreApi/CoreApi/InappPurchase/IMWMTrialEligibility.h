#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, MWMCheckTrialEligibilityResult) {
  MWMCheckTrialEligibilityResultEligible,
  MWMCheckTrialEligibilityResultNotEligible,
  MWMCheckTrialEligibilityResultServerError,
  MWMCheckTrialEligibilityResultNoReceipt
};

typedef void (^CheckTrialEligibilityCallback)(MWMCheckTrialEligibilityResult result);

@protocol IMWMTrialEligibility <NSObject>

- (void)checkTrialEligibility:(NSString *)serverId callback:(CheckTrialEligibilityCallback)callback;

@end

NS_ASSUME_NONNULL_END
