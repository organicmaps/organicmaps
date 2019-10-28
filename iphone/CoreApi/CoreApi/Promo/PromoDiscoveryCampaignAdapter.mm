#import "PromoDiscoveryCampaignAdapter.h"
#include "map/onboarding.hpp"

@implementation PromoDiscoveryCampaignAdapter

- (instancetype)init {
  self = [super init];
  if (self) {
    onboarding::Tip tip = onboarding::GetTip();
    NSString* urlStr = [[NSString alloc] initWithUTF8String:tip.m_url.c_str()];
    _url = [[NSURL alloc] initWithString:urlStr];
    _type = static_cast<NSInteger>(tip.m_type);
  }

  return self;
}

- (BOOL)canShowTipButton {
  return onboarding::CanShowTipButton();
}

@end
