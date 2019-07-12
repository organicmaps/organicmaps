#include "partners_api/promo_api.hpp"

NS_ASSUME_NONNULL_BEGIN

@interface MWMPromoAfterBooking : NSObject

@property(nonatomic, readonly) NSString *promoId;
@property(nonatomic, readonly) NSString *promoUrl;
@property(nonatomic, readonly) NSString *pictureUrl;

- (instancetype)initWithPromoAfterBooking:(promo::AfterBooking const &)afterBooking;

@end

NS_ASSUME_NONNULL_END
