import MoPub_FacebookAudienceNetwork_Adapters

final class MopubBanner: NSObject, Banner {
  private enum Limits {
    static let minTimeOnScreen: TimeInterval = 3
    static let minTimeSinceLastRequest: TimeInterval = 5
  }

  fileprivate var success: Banner.Success!
  fileprivate var failure: Banner.Failure!
  fileprivate var click: Banner.Click!
  private var requestDate: Date?
  private var showDate: Date?
  private var remainingTime = Limits.minTimeOnScreen

  private let placementID: String

  func reload(success: @escaping Banner.Success, failure: @escaping Banner.Failure, click: @escaping Click) {
    self.success = success
    self.failure = failure
    self.click = click

    load()
    requestDate = Date()
  }

  func unregister() {
    nativeAd?.unregister()
  }

  var isBannerOnScreen = false {
    didSet {
      if isBannerOnScreen {
        startCountTimeOnScreen()
      } else {
        stopCountTimeOnScreen()
      }
    }
  }

  private(set) var isNeedToRetain = false
  var isPossibleToReload: Bool {
    if let date = requestDate {
      return Date().timeIntervalSince(date) > Limits.minTimeSinceLastRequest
    }
    return true
  }

  var type: BannerType { return .mopub(bannerID) }
  var mwmType: MWMBannerType { return type.mwmType }
  var bannerID: String { return placementID }

  var statisticsDescription: [String: String] {
    return [kStatBanner: bannerID, kStatProvider: kStatMopub]
  }

  init(bannerID: String) {
    placementID = bannerID
    super.init()

    let center = NotificationCenter.default
    center.addObserver(self,
                       selector: #selector(enterForeground),
                       name: UIApplication.willEnterForegroundNotification,
                       object: nil)
    center.addObserver(self,
                       selector: #selector(enterBackground),
                       name: UIApplication.didEnterBackgroundNotification,
                       object: nil)
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
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

  private func startCountTimeOnScreen() {
    if showDate == nil {
      showDate = Date()
    }

    if remainingTime > 0 {
      perform(#selector(setEnoughTimeOnScreen), with: nil, afterDelay: remainingTime)
    }
  }

  private func stopCountTimeOnScreen() {
    guard let date = showDate else {
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

  // MARK: - Content
  private(set) var nativeAd: MPNativeAd?

  var title: String {
    return nativeAd?.properties[kAdTitleKey] as? String ?? ""
  }

  var text: String {
    return nativeAd?.properties[kAdTextKey] as? String ?? ""
  }

  var iconURL: String {
    return nativeAd?.properties[kAdIconImageKey] as? String ?? ""
  }

  var ctaText: String {
    return nativeAd?.properties[kAdCTATextKey] as? String ?? ""
  }

  var privacyInfoURL: URL? {
    guard let nativeAd = nativeAd else { return nil }

    if nativeAd.adAdapter is FacebookNativeAdAdapter {
      return (nativeAd.adAdapter as! FacebookNativeAdAdapter).fbNativeAdBase.adChoicesLinkURL
    }

    return URL(string: kPrivacyIconTapDestinationURL)
  }

  // MARK: - Helpers
  private var request: MPNativeAdRequest!

  private func load() {
    let settings = MPStaticNativeAdRendererSettings()
    let config = MPStaticNativeAdRenderer.rendererConfiguration(with: settings)!
    let fbConfig = FacebookNativeAdRenderer.rendererConfiguration(with: settings)
    request = MPNativeAdRequest(adUnitIdentifier: placementID, rendererConfigurations: [config, fbConfig])
    let targeting = MPNativeAdRequestTargeting()
    targeting?.keywords = "user_lang:\(AppInfo.shared().twoLetterLanguageId)"
    targeting?.desiredAssets = [kAdTitleKey, kAdTextKey, kAdIconImageKey, kAdCTATextKey]
    if let location = LocationManager.lastLocation() {
      targeting?.location = location
    }
    request.targeting = targeting

    request.start { [weak self] _, nativeAd, error in
      guard let s = self else { return }
      if let error = error as NSError? {
        let params: [String: Any] = [
          kStatBanner: s.bannerID,
          kStatProvider: kStatMopub,
        ]
        let event = kStatPlacePageBannerError
        s.failure(s.type, event, params, error)
      } else {
        nativeAd?.delegate = self
        s.nativeAd = nativeAd
        s.success(s)
      }
    }
  }
}

extension MopubBanner: MPNativeAdDelegate {
  func willPresentModal(for nativeAd: MPNativeAd!) {
    guard nativeAd === self.nativeAd else { return }
    click(self)
  }

  func viewControllerForPresentingModalView() -> UIViewController! {
    return UIViewController.topViewController()
  }
}
