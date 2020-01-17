#import "PromoAfterBookingData+Core.h"

@implementation PromoAfterBookingData

- (instancetype)initWithAfterBooking:(promo::AfterBooking)afterBooking {
  self = [super init];
  if (self) {
    _promoId = @(afterBooking.m_id.c_str());
    _promoUrl = @(afterBooking.m_promoUrl.c_str());
    _pictureUrl = @(afterBooking.m_pictureUrl.c_str());
    _enabled = afterBooking.IsEmpty() == false;
  }
  return self;
}

- (instancetype)init
{
  self = [super init];
  if (self) {
    _promoId = @"";
    _promoUrl = @"";
    _pictureUrl = @"";
    _enabled = false;
  }
  return self;
}

@end
