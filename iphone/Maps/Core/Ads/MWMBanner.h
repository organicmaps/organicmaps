typedef NS_ENUM(NSInteger, MWMBannerType) {
  MWMBannerTypeNone,
  MWMBannerTypeFacebook,
  MWMBannerTypeRb,
  MWMBannerTypeMopub,
  MWMBannerTypeGoogle
};

@protocol MWMBanner <NSObject>
@property(nonatomic, readonly) enum MWMBannerType mwmType;
@property(copy, nonatomic, readonly) NSString * bannerID;
@end
