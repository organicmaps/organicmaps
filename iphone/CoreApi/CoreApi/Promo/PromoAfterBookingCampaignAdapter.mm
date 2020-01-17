#import "PromoAfterBookingCampaignAdapter.h"
#import "PromoAfterBookingData+Core.h"
#import <CoreApi/Framework.h>
#import "platform/network_policy.hpp"

@implementation PromoAfterBookingCampaignAdapter

+ (PromoAfterBookingData*)afterBookingData {
  auto policy = platform::GetCurrentNetworkPolicy();
  auto promoApi = GetFramework().GetPromoApi(policy);
  if (promoApi != nil) {
    auto const promoAfterBooking = promoApi->GetAfterBooking(languages::GetCurrentNorm());
    return [[PromoAfterBookingData alloc] initWithAfterBooking:promoAfterBooking];
  }
  return [[PromoAfterBookingData alloc] init];
}

@end
