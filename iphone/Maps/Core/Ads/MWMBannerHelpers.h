#import <CoreApi/CoreBanner.h>

#include "partners_api/ads/banner.hpp"

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
  case ads::Banner::Type::TinkoffAllAirlines: return MWMBannerTypeTinkoffAllAirlines;
  case ads::Banner::Type::TinkoffInsurance: return MWMBannerTypeTinkoffInsurance;
  case ads::Banner::Type::Mts: return MWMBannerTypeMts;
  case ads::Banner::Type::Skyeng: return MWMBannerTypeSkyeng;
  case ads::Banner::Type::BookmarkCatalog: return MWMBannerTypeBookmarkCatalog;
  case ads::Banner::Type::MastercardSberbank: return MWMBannerTypeMastercardSberbank;
  case ads::Banner::Type::Citymobil: return MWMBannerTypeCitymobil;
  case ads::Banner::Type::ArsenalMedic: return MWMBannerTypeArsenalMedic;
  case ads::Banner::Type::ArsenalFlat: return MWMBannerTypeArsenalFlat;
  case ads::Banner::Type::ArsenalInsuranceCrimea: return MWMBannerTypeArsenalInsuranceCrimea;
  case ads::Banner::Type::ArsenalInsuranceRussia: return MWMBannerTypeArsenalInsuranceRussia;
  case ads::Banner::Type::ArsenalInsuranceWorld: return MWMBannerTypeArsenalInsuranceWorld;
  }
}

static inline CoreBanner * MatchBanner(ads::Banner const & banner, NSString * query)
{
  return [[CoreBanner alloc] initWithMwmType:MatchBannerType(banner.m_type)
                                    bannerID:@(banner.m_value.c_str())
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
