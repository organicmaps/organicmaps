typedef NS_ENUM(NSInteger, MWMBannerType) {
  MWMBannerTypeNone = 0,
  MWMBannerTypeFacebook = 1,
  MWMBannerTypeRb = 2,
};

@protocol MWMBanner
@property(nonatomic, readonly) enum MWMBannerType mwmType;
@property(copy, nonatomic, readonly) NSString * bannerID;
@end

@interface MWMCoreBanner : NSObject<MWMBanner>

- (instancetype)initWith:(MWMBannerType)type bannerID:(NSString *)bannerID;

@property(nonatomic, readonly) enum MWMBannerType mwmType;
@property(copy, nonatomic, readonly) NSString * bannerID;

@end
