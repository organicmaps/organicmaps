@objc(MWMCoreBanner)
final class CoreBanner: NSObject, MWMBanner {
  let mwmType: MWMBannerType
  let bannerID: String

  @objc init(mwmType: MWMBannerType, bannerID: String) {
    self.mwmType = mwmType
    self.bannerID = bannerID
  }
}
