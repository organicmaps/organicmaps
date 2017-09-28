import GoogleMobileAds

final class GoogleNativeBanner: NSObject, Banner {
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
  var bannerID: String! { return adLoader.adUnitID }
  var statisticsDescription: [String: String] {
    return [kStatBanner: bannerID, kStatProvider: kStatGoogle]
  }
  private let query: String
  private let adLoader: GADAdLoader

  fileprivate var nativeAd: GADNativeAd!

  init(bannerID _: String, query: String) {
    self.query = query
    adLoader = GADAdLoader(adUnitID: "ca-app-pub-6656946757675080/8014770099",
                           rootViewController: UIViewController.topViewController(),
                           adTypes: [GADAdLoaderAdType.nativeAppInstall, GADAdLoaderAdType.nativeContent],
                           options: nil)
    super.init()
    adLoader.delegate = self
  }

  func reload(success: @escaping Success, failure: @escaping Failure, click: @escaping Click) {
    self.success = success
    self.failure = failure
    self.click = click

    let request = GADSearchRequest()
    request.testDevices = [kGADSimulatorID]
    request.query = query
    if let loc = MWMLocationManager.lastLocation() {
      request.setLocationWithLatitude(CGFloat(loc.coordinate.latitude),
                                      longitude: CGFloat(loc.coordinate.longitude),
                                      accuracy: CGFloat(loc.horizontalAccuracy))
    }

    adLoader.load(request)
    requestDate = Date()
  }

  func unregister() {
    switch nativeAd {
    case let nativeAppInstallAd as GADNativeAppInstallAd: nativeAppInstallAd.unregisterAdView()
    case let nativeContentAd as GADNativeContentAd: nativeContentAd.unregisterAdView()
    default: assert(false)
    }
  }
}

extension GoogleNativeBanner: GADAdLoaderDelegate {
  func adLoader(_: GADAdLoader, didFailToReceiveAdWithError error: GADRequestError) {
    var params: [String: Any] = statisticsDescription
    params[kStatErrorCode] = error.code

    failure(type, kStatPlacePageBannerError, params, error)
  }
}

extension GoogleNativeBanner: GADNativeAppInstallAdLoaderDelegate {
  func adLoader(_: GADAdLoader, didReceive nativeAppInstallAd: GADNativeAppInstallAd) {
    nativeAd = nativeAppInstallAd
    success(self)
  }
}

extension GoogleNativeBanner: GADNativeContentAdLoaderDelegate {
  func adLoader(_: GADAdLoader, didReceive nativeContentAd: GADNativeContentAd) {
    nativeAd = nativeContentAd
    success(self)
  }
}
