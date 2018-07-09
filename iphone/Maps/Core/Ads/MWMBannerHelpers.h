#import "MWMBanner.h"
#import "SwiftBridge.h"

#include "partners_api/banner.hpp"

#include <vector>

namespace banner_helpers
{
static inline MWMBannerType MatchBannerType(ads::Banner::Type coreType)
{
  switch (coreType)
  {
  case ads::Banner::Type::None: return MWMBannerTypeNone;
  case ads::Banner::Type::Facebook: return MWMBannerTypeFacebook;
  case ads::Banner::Type::RB: return MWMBannerTypeRb;
  case ads::Banner::Type::Mopub: return MWMBannerTypeMopub;
  case ads::Banner::Type::Google: return MWMBannerTypeGoogle;
  }
}

static inline MWMCoreBanner * MatchBanner(ads::Banner const & banner, NSString * query)
{
  return [[MWMCoreBanner alloc] initWithMwmType:MatchBannerType(banner.m_type)
                                       bannerID:@(banner.m_bannerId.c_str())
                                          query:query];
}

static inline NSArray<MWMCoreBanner *> * MatchPriorityBanners(std::vector<ads::Banner> const & banners, NSString * query = @"")
{
  NSMutableArray<MWMCoreBanner *> * mBanners = [@[] mutableCopy];
  for (auto const & banner : banners)
    [mBanners addObject:MatchBanner(banner, query)];
  return [mBanners copy];
}
}
