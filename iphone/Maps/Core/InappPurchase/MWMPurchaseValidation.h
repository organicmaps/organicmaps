#import "IMWMPurchaseValidation.h"

NS_ASSUME_NONNULL_BEGIN

@interface MWMPurchaseValidation : NSObject <IMWMPurchaseValidation>

- (instancetype)init __unavailable;
- (instancetype)initWithVendorId:(NSString *)vendorId;

@end

NS_ASSUME_NONNULL_END
