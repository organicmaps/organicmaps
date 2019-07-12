#import "MWMPromoAfterBooking.h"

@interface MWMPromoAfterBooking() {
  promo::AfterBooking m_afterBooking;
}
@end

@implementation MWMPromoAfterBooking

- (instancetype)initWithPromoAfterBooking:(promo::AfterBooking const &)afterBooking {
  self = [super init];
  if (self) {
    m_afterBooking = afterBooking;
  }
  return self;
}

- (NSString *)promoId {
  return @(m_afterBooking.m_id.c_str());
}

- (NSString *)promoUrl {
  return @(m_afterBooking.m_promoUrl.c_str());
}

- (NSString *)pictureUrl {
  return @(m_afterBooking.m_pictureUrl.c_str());
}

@end

