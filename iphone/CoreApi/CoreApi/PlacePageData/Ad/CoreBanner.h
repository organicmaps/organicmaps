#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, MWMBannerType) {
  MWMBannerTypeNone,
  MWMBannerTypeFacebook,
  MWMBannerTypeRb,
  MWMBannerTypeMopub,
  MWMBannerTypeTinkoffAllAirlines,
  MWMBannerTypeTinkoffInsurance,
  MWMBannerTypeMts,
  MWMBannerTypeSkyeng,
  MWMBannerTypeBookmarkCatalog,
  MWMBannerTypeMastercardSberbank,
  MWMBannerTypeCitymobil,
  MWMBannerTypeArsenalMedic,
  MWMBannerTypeArsenalFlat,
  MWMBannerTypeArsenalInsuranceCrimea,
  MWMBannerTypeArsenalInsuranceRussia,
  MWMBannerTypeArsenalInsuranceWorld,
};

NS_ASSUME_NONNULL_BEGIN

@protocol MWMBanner <NSObject>

@property(nonatomic, readonly) enum MWMBannerType mwmType;
@property(nonatomic, readonly) NSString *bannerID;

@end

@interface CoreBanner : NSObject <MWMBanner>

@property(nonatomic, readonly) MWMBannerType mwmType;
@property(nonatomic, readonly) NSString *bannerID;
@property(nonatomic, readonly) NSString *query;

- (instancetype)initWithMwmType:(MWMBannerType)type bannerID:(NSString *)bannerID query:(NSString *)query;

@end

NS_ASSUME_NONNULL_END
