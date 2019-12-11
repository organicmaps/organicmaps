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
    _bannerID = @(banner.m_bannerId.c_str());
    _query = @"";
  }
  return self;
}

@end
