#import "PromoAfterBookingCampaignAdapter.h"

#include <CoreApi/Framework.h>
#include "platform/network_policy.hpp"

@interface PromoAfterBookingCampaignAdapter () {
  promo::AfterBooking m_afterBooking;
}
@end

@implementation PromoAfterBookingCampaignAdapter

- (instancetype)init {
  self = [super init];
  if (self) {
    auto policy = platform::GetCurrentNetworkPolicy();
    auto promoApi = GetFramework().GetPromoApi(policy);
    if (promoApi == nullptr)
      return nil;

    auto const promoAfterBooking = promoApi->GetAfterBooking(languages::GetCurrentNorm());

    if (promoAfterBooking.IsEmpty())
      return nil;

    m_afterBooking = promoAfterBooking;
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
