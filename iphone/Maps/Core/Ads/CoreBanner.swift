@objc(MWMCoreBanner)
final class CoreBanner: NSObject, MWMBanner {
  let mwmType: MWMBannerType
  let bannerID: String
  let query: String

  @objc init(mwmType: MWMBannerType, bannerID: String, query: String) {
    self.mwmType = mwmType
    self.bannerID = bannerID
    self.query = query
  }

  override var debugDescription: String {
    let type: String
    switch mwmType {
    case .none: type = "none"
    case .facebook: type = "facebook"
    case .rb: type = "rb"
    case .mopub: type = "mopub"
    case .google: type = "google"
    }
    return "Type: <\(type)> | id: <\(bannerID)> | query: <\(query)>"
  }
}
