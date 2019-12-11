#import <CoreApi/CoreBanner.h>

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
  }
}

static inline CoreBanner * MatchBanner(ads::Banner const & banner, NSString * query)
{
  return [[CoreBanner alloc] initWithMwmType:MatchBannerType(banner.m_type)
                                    bannerID:@(banner.m_bannerId.c_str())
                                       query:query];
}

static inline NSArray<CoreBanner *> * MatchPriorityBanners(std::vector<ads::Banner> const & banners, NSString * query = @"")
{
  NSMutableArray<CoreBanner *> * mBanners = [@[] mutableCopy];
  for (auto const & banner : banners)
    [mBanners addObject:MatchBanner(banner, query)];
  return [mBanners copy];
}
}
