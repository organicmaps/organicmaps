#import "MWMPromoApi.h"
#import "MWMPromoAfterBooking.h"

#include <CoreApi/Framework.h>

#include "platform/network_policy.hpp"
#include "platform/preferred_languages.hpp"

@implementation MWMPromoApi

+ (MWMPromoAfterBooking *)afterBooking {
  auto policy = platform::GetCurrentNetworkPolicy();
  auto promoApi = GetFramework().GetPromoApi(policy);
  if (promoApi == nullptr)
    return nil;
  
  auto const promoAfterBooking = promoApi->GetAfterBooking(languages::GetCurrentNorm());

  if (promoAfterBooking.IsEmpty())
    return nil;
  
  return [[MWMPromoAfterBooking alloc] initWithPromoAfterBooking:promoAfterBooking];
}

@end
