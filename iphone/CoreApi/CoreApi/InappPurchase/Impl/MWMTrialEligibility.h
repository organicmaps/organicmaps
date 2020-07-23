#import "IMWMTrialEligibility.h"

NS_ASSUME_NONNULL_BEGIN

@interface MWMTrialEligibility : NSObject <IMWMTrialEligibility>

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithVendorId:(NSString *)vendorId;

@end

NS_ASSUME_NONNULL_END
