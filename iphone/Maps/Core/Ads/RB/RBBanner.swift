final class RBBanner: MTRGNativeAd, Banner {
  private enum Limits {
    static let minTimeOnScreen: TimeInterval = 3
    static let minTimeSinceLastRequest: TimeInterval = 5
  }

  fileprivate enum Settings {
    static let placementIDKey = "_SITEZONE"
  }

  fileprivate var success: Banner.Success!
  fileprivate var failure: Banner.Failure!
  fileprivate var click: Banner.Click!
  fileprivate var requestDate: Date?
  private var showDate: Date?
  private var remainingTime = Limits.minTimeOnScreen

  init!(bannerID: String) {
    super.init(slotId: UInt(MY_TARGET_RB_KEY))
    delegate = self
    self.bannerID = bannerID

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

  // MARK: - Banner
  func reload(success: @escaping Banner.Success, failure: @escaping Banner.Failure, click: @escaping Click) {
    self.success = success
    self.failure = failure
    self.click = click

    load()
    requestDate = Date()
  }

  func unregister() {
    unregisterView()
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

  var type: BannerType { return .rb(bannerID) }
  var mwmType: MWMBannerType { return type.mwmType }
  var bannerID: String! {
    get {
      return customParams.customParam(forKey: Settings.placementIDKey)
    }
    set {
      customParams.setCustomParam(newValue, forKey: Settings.placementIDKey)
    }
  }

  var statisticsDescription: [String: String] {
    return [kStatBanner: bannerID, kStatProvider: kStatRB]
  }
}

extension RBBanner: MTRGNativeAdDelegate {
  func onLoad(with _: MTRGNativePromoBanner!, nativeAd: MTRGNativeAd!) {
    guard nativeAd === self else { return }
    success(self)
  }

  func onNoAd(withReason reason: String!, nativeAd: MTRGNativeAd!) {
    guard nativeAd === self else { return }
    let params: [String: Any] = [
      kStatBanner: bannerID ?? "",
      kStatProvider: kStatRB,
      kStatReason: reason ?? "",
    ]
    let event = kStatPlacePageBannerError
    let error = NSError(domain: kMapsmeErrorDomain, code: 1001, userInfo: params)
    failure(self.type, event, params, error)
  }

  func onAdClick(with nativeAd: MTRGNativeAd!) {
    guard nativeAd === self else { return }
    click(self)
  }
}
