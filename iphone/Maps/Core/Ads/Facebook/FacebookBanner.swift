import FBAudienceNetwork

// MARK: FacebookBanner
final class FacebookBanner: NSObject, Banner {
  private enum Limits {
    static let minTimeOnScreen: TimeInterval = 3
    static let minTimeSinceLastRequest: TimeInterval = 5
  }

  fileprivate var success: Banner.Success!
  fileprivate var failure: Banner.Failure!
  fileprivate var click: Banner.Click!

  let nativeAd: FBNativeAd

  func reload(success: @escaping Banner.Success, failure: @escaping Banner.Failure, click: @escaping Click) {
    FBAdSettings.clearTestDevices()
    self.success = success
    self.failure = failure
    self.click = click

    nativeAd.load()
    requestDate = Date()
  }

  func unregister() {
    nativeAd.unregisterView()
  }

  var isPossibleToReload: Bool {
    if let date = requestDate {
      return Date().timeIntervalSince(date) > Limits.minTimeSinceLastRequest
    }
    return true
  }

  private(set) var isNeedToRetain: Bool = true
  var type: BannerType { return .facebook(bannerID) }
  var mwmType: MWMBannerType { return type.mwmType }
  var bannerID: String! { return nativeAd.placementID }
  var isBannerOnScreen = false {
    didSet {
      if isBannerOnScreen {
        startCountTimeOnScreen()
      } else {
        stopCountTimeOnScreen()
      }
    }
  }

  var statisticsDescription: [String: String] {
    return [kStatBanner: bannerID, kStatProvider: kStatFacebook]
  }

  // MARK: Helpers
  private var requestDate: Date?
  private var remainingTime = Limits.minTimeOnScreen
  private var loadBannerDate: Date?

  private func startCountTimeOnScreen() {
    if loadBannerDate == nil {
      loadBannerDate = Date()
    }

    if remainingTime > 0 {
      perform(#selector(setEnoughTimeOnScreen), with: nil, afterDelay: remainingTime)
    }
  }

  private func stopCountTimeOnScreen() {
    guard let date = loadBannerDate else {
      assert(false)
      return
    }

    let timePassed = Date().timeIntervalSince(date)
    if timePassed < Limits.minTimeOnScreen {
      remainingTime = Limits.minTimeOnScreen - timePassed
      NSObject.cancelPreviousPerformRequests(withTarget: self)
    } else {
      remainingTime = 0
    }
  }

  @objc private func setEnoughTimeOnScreen() {
    isNeedToRetain = false
  }

  init(bannerID: String) {
    nativeAd = FBNativeAd(placementID: bannerID)
    nativeAd.mediaCachePolicy = .all
    super.init()
    nativeAd.delegate = self
    let center = NotificationCenter.default
    center.addObserver(self,
                       selector: #selector(enterForeground),
                       name: .UIApplicationWillEnterForeground,
                       object: nil)
    center.addObserver(self,
                       selector: #selector(enterBackground),
                       name: .UIApplicationDidEnterBackground,
                       object: nil)
  }

  @objc private func enterForeground() {
    if isBannerOnScreen {
      startCountTimeOnScreen()
    }
  }

  @objc private func enterBackground() {
    if isBannerOnScreen {
      stopCountTimeOnScreen()
    }
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }
}

// MARK: FacebookBanner: FBNativeAdDelegate
extension FacebookBanner: FBNativeAdDelegate {

  func nativeAdDidLoad(_ nativeAd: FBNativeAd) {
    guard nativeAd === self.nativeAd else { return }
    success(self)
  }

  func nativeAd(_ nativeAd: FBNativeAd, didFailWithError error: Error) {
    guard nativeAd === self.nativeAd else { return }

    // https://developers.facebook.com/docs/audience-network/testing
    var params: [String: Any] = statisticsDescription

    let e = error as NSError
    let event: String
    if e.code == 1001 {
      event = kStatPlacePageBannerBlank
    } else {
      event = kStatPlacePageBannerError
      params[kStatErrorCode] = e.code
    }

    failure(type, event, params, e)
  }

  func nativeAdDidClick(_ nativeAd: FBNativeAd) {
    guard nativeAd === self.nativeAd else { return }
    click(self)
  }
}
