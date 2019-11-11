#import "HotelBookingData.h"

#include <CoreApi/Framework.h>

NS_ASSUME_NONNULL_BEGIN

@interface HotelBookingData (Core)

- (instancetype)initWithHotelInfo:(booking::HotelInfo const &)hotelInfo;

@end

NS_ASSUME_NONNULL_END
