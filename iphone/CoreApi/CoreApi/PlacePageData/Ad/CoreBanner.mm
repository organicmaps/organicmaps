#import "CoreBanner+Core.h"

static MWMBannerType ConvertBannerType(ads::Banner::Type coreType) {
  switch (coreType) {
    case ads::Banner::Type::None:
      return MWMBannerTypeNone;
    case ads::Banner::Type::Facebook:
      return MWMBannerTypeFacebook;
    case ads::Banner::Type::RB:
      return MWMBannerTypeRb;
    case ads::Banner::Type::Mopub:
      return MWMBannerTypeMopub;
    case ads::Banner::Type::TinkoffAllAirlines:
      return MWMBannerTypeTinkoffAllAirlines;
    case ads::Banner::Type::TinkoffInsurance:
      return MWMBannerTypeTinkoffInsurance;
    case ads::Banner::Type::Mts:
      return MWMBannerTypeMts;
    case ads::Banner::Type::Skyeng:
      return MWMBannerTypeSkyeng;
    case ads::Banner::Type::BookmarkCatalog:
      return MWMBannerTypeBookmarkCatalog;
    case ads::Banner::Type::MastercardSberbank:
      return MWMBannerTypeMastercardSberbank;
    case ads::Banner::Type::Citymobil:
      return MWMBannerTypeCitymobil;
    case ads::Banner::Type::ArsenalMedic:
      return MWMBannerTypeArsenalMedic;
    case ads::Banner::Type::ArsenalFlat:
      return MWMBannerTypeArsenalFlat;
    case ads::Banner::Type::ArsenalInsuranceCrimea:
      return MWMBannerTypeArsenalInsuranceCrimea;
    case ads::Banner::Type::ArsenalInsuranceRussia:
      return MWMBannerTypeArsenalInsuranceRussia;
    case ads::Banner::Type::ArsenalInsuranceWorld:
      return MWMBannerTypeArsenalInsuranceWorld;
  }
}

@interface CoreBanner ()

@property(nonatomic, readwrite) MWMBannerType mwmType;
@property(nonatomic, copy) NSString *bannerID;
@property(nonatomic, copy) NSString *query;

@end

@implementation CoreBanner

- (instancetype)initWithMwmType:(MWMBannerType)type bannerID:(NSString *)bannerID query:(NSString *)query {
  self = [super init];
  if (self) {
    _mwmType = type;
    _bannerID = bannerID;
    _query = query;
  }
  return self;
}

@end

@implementation CoreBanner (Core)

- (instancetype)initWithAdBanner:(ads::Banner const &)banner {
  self = [super init];
  if (self) {
    _mwmType = ConvertBannerType(banner.m_type);
    _bannerID = @(banner.m_value.c_str());
    _query = @"";
  }
  return self;
}

@end
