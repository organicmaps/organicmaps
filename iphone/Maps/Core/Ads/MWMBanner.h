typedef NS_ENUM(NSInteger, MWMBannerType) {
  MWMBannerTypeNone,
  MWMBannerTypeFacebook,
  MWMBannerTypeRb,
  MWMBannerTypeMopub,
  MWMBannerTypeGoogle
};

@protocol MWMBanner
@property(nonatomic, readonly) enum MWMBannerType mwmType;
@property(nonatomic, readonly) NSString * bannerID;
@end
