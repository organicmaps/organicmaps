#include <CoreApi/Framework.h>
#import "PromoAfterBookingData.h"

NS_ASSUME_NONNULL_BEGIN

@interface PromoAfterBookingData (Core)

- (instancetype)initWithAfterBooking:(promo::AfterBooking)afterBooking;

@end

NS_ASSUME_NONNULL_END
