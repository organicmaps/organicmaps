import GoogleMobileAds

@objc(MWMGoogleFallbackBannerDynamicSizeDelegate)
protocol GoogleFallbackBannerDynamicSizeDelegate {
  func dynamicSizeUpdated(banner: GoogleFallbackBanner)
}

@objc(MWMGoogleFallbackBanner)
final class GoogleFallbackBanner: GADSearchBannerView, Banner {
  private enum Limits {
    static let minTimeSinceLastRequest: TimeInterval = 5
  }

  fileprivate var success: Banner.Success!
  fileprivate var failure: Banner.Failure!
  fileprivate var click: Banner.Click!

  private var requestDate: Date?
  var isBannerOnScreen: Bool = false
  var isNeedToRetain: Bool { return true }
  var isPossibleToReload: Bool {
    if let date = requestDate {
      return Date().timeIntervalSince(date) > Limits.minTimeSinceLastRequest
    }
    return true
  }
  var type: BannerType { return .google(bannerID, query) }
  var mwmType: MWMBannerType { return type.mwmType }
  var bannerID: String! { return adUnitID }
  var statisticsDescription: [String: String] {
    return [kStatBanner: bannerID, kStatProvider: kStatGoogle]
  }
  let query: String

  @objc var dynamicSizeDelegate: GoogleFallbackBannerDynamicSizeDelegate?
  fileprivate(set) var dynamicSize = CGSize.zero {
    didSet {
      dynamicSizeDelegate?.dynamicSizeUpdated(banner: self)
    }
  }
  @objc var cellIndexPath: IndexPath!

  init(bannerID: String, query: String) {
    self.query = query
    super.init(adSize: kGADAdSizeFluid)
    adUnitID = bannerID
    frame = CGRect.zero

    delegate = self
    adSizeDelegate = self
  }

  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func reload(success: @escaping Success, failure: @escaping Failure, click: @escaping Click) {
    self.success = success
    self.failure = failure
    self.click = click

    let searchRequest = GADDynamicHeightSearchRequest()
    searchRequest.query = query
    searchRequest.numberOfAds = 1
    if let loc = MWMLocationManager.lastLocation() {
      searchRequest.setLocationWithLatitude(CGFloat(loc.coordinate.latitude),
                                            longitude: CGFloat(loc.coordinate.longitude),
                                            accuracy: CGFloat(loc.horizontalAccuracy))
    }
    searchRequest.cssWidth = "100%"
    let isNightMode = UIColor.isNightMode()
    searchRequest.adBorderCSSSelections = "bottom"
    searchRequest.adBorderColor = isNightMode ? "#53575A" : "#E0E0E0"
    searchRequest.adjustableLineHeight = 18
    searchRequest.annotationFontSize = 12
    searchRequest.annotationTextColor = isNightMode ? "#C8C9CA" : "#75726D"
    searchRequest.attributionBottomSpacing = 4
    searchRequest.attributionFontSize = 12
    searchRequest.attributionTextColor = isNightMode ? "#C8C9CA" : "#75726D"
    searchRequest.backgroundColor = isNightMode ? "#484B50" : "#FFF9EF"
    searchRequest.boldTitleEnabled = true
    searchRequest.clickToCallExtensionEnabled = true
    searchRequest.descriptionFontSize = 12
    searchRequest.domainLinkColor = isNightMode ? "#51B5E6" : "#1E96F0"
    searchRequest.domainLinkFontSize = 12
    searchRequest.locationExtensionEnabled = true
    searchRequest.locationExtensionFontSize = 12
    searchRequest.locationExtensionTextColor = isNightMode ? "#C8C9CA" : "#75726D"
    searchRequest.sellerRatingsExtensionEnabled = false
    searchRequest.siteLinksExtensionEnabled = false
    searchRequest.textColor = isNightMode ? "#C8C9CA" : "#75726D"
    searchRequest.titleFontSize = 12
    searchRequest.titleLinkColor = isNightMode ? "#FFFFFF" : "#21201E"
    searchRequest.titleUnderlineHidden = true

    load(searchRequest)
    requestDate = Date()
  }

  func unregister() {}

  static func ==(lhs: GoogleFallbackBanner, rhs: GoogleFallbackBanner) -> Bool {
    return lhs.adUnitID == rhs.adUnitID && lhs.query == rhs.query
  }
}

extension GoogleFallbackBanner: GADBannerViewDelegate {
  func adViewDidReceiveAd(_ bannerView: GADBannerView) {
    guard let banner = bannerView as? GoogleFallbackBanner, banner == self else { return }
    success(self)
  }

  func adView(_ bannerView: GADBannerView, didFailToReceiveAdWithError error: GADRequestError) {
    guard let banner = bannerView as? GoogleFallbackBanner, banner == self else { return }
    var params: [String: Any] = statisticsDescription
    params[kStatErrorCode] = error.code

    failure(type, kStatPlacePageBannerError, params, error)
  }

  func adViewWillPresentScreen(_: GADBannerView) {
    click(self)
  }

  func adViewWillLeaveApplication(_: GADBannerView) {
    click(self)
  }
}

extension GoogleFallbackBanner: GADAdSizeDelegate {
  func adView(_: GADBannerView, willChangeAdSizeTo size: GADAdSize) {
    var newFrame = frame
    newFrame.size.height = size.size.height
    frame = newFrame
    dynamicSize = size.size
  }
}
