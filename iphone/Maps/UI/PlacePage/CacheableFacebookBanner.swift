import FBAudienceNetwork

// MARK: CacheableFacebookBanner
final class CacheableFacebookBanner: FBNativeAd, Cacheable {

  fileprivate var success: Cacheable.Success!
  fileprivate var failure: Cacheable.Failure!

  func reload(_ suc: @escaping Cacheable.Success, failure fail: @escaping Cacheable.Failure) {
    success = suc
    failure = fail

    mediaCachePolicy = .all
    delegate = self
    load()
    requestDate = Date()
  }

  func bannerIsOutOfScreen() {
    stopCountTimeOnScreen()
    isBannerOnScreen = false
  }

  func bannerIsOnScreen() {
    isBannerOnScreen = true
    startCountTimeOnScreen()
  }

  var isPossibleToReload: Bool {
    if let date = requestDate {
      return Date().timeIntervalSince(date) > Limits.minTimeSinceLastRequest
    }
    return true
  }

  var isNeedToRetain: Bool = true
  var adID: String { return placementID }
  private(set) var isBannerOnScreen = false

  // MARK: Helpers
  private var requestDate: Date?
  private var remainingTime = Limits.minTimeOnScreen
  private var loadBannerDate: Date?

  private enum Limits {
    static let minTimeOnScreen: TimeInterval = 3
    static let minTimeSinceLastRequest: TimeInterval = 30
  }

  private func startCountTimeOnScreen() {
    if loadBannerDate == nil {
      loadBannerDate = Date()
    }

    if (remainingTime > 0) {
      perform(#selector(setEnoughTimeOnScreen), with: nil, afterDelay: remainingTime)
    }
  }

  private func stopCountTimeOnScreen() {
    guard let date = loadBannerDate else {
      assert(false)
      return
    }

    let timePassed = Date().timeIntervalSince(date)
    if (timePassed < Limits.minTimeOnScreen) {
      remainingTime = Limits.minTimeOnScreen - timePassed
      NSObject.cancelPreviousPerformRequests(withTarget: self)
    } else {
      remainingTime = 0
    }
  }

  @objc private func setEnoughTimeOnScreen() {
    isNeedToRetain = false
  }

  override init(placementID: String) {
    super.init(placementID: placementID)
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
    if (isBannerOnScreen) {
      stopCountTimeOnScreen()
    }
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

}

// MARK: CacheableFaceebookBanner: FBNativeAdDelegate
extension CacheableFacebookBanner: FBNativeAdDelegate {

  func nativeAdDidLoad(_ nativeAd: FBNativeAd) {
    success(self)
  }

  func nativeAd(_ nativeAd: FBNativeAd, didFailWithError error: Error) {

    // https://developers.facebook.com/docs/audience-network/testing
    var params: [String: Any] = [kStatBanner : nativeAd.placementID, kStatProvider : kStatFacebook]

    let e = error as NSError
    let event: String
    if e.code == 1001 {
      event = kStatPlacePageBannerBlank
    } else {
      event = kStatPlacePageBannerError
      params[kStatErrorCode] = e.code

      var message: String = ""
      for (k, v) in e.userInfo {
        message += "\(k) : \(v)\n"
      }

      params[kStatErrorMessage] = message
    }
    
    failure(event, params, e)
  }
}

