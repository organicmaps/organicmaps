typedef NS_ENUM(NSInteger, MWMBannerType) {
  MWMBannerTypeNone,
  MWMBannerTypeFacebook,
  MWMBannerTypeRb,
};

@protocol MWMBanner
@property(nonatomic, readonly) enum MWMBannerType mwmType;
@property(nonatomic, readonly) NSString * bannerID;
@end
