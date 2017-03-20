import Crashlytics

// MARK: Cacheable protocol
protocol Cacheable {

  typealias EventName = String
  typealias ErrorDetails = [String : Any]
  typealias Success = (Cacheable) -> Void
  typealias Failure = (EventName, ErrorDetails, NSError) -> Void

  func reload(_ success: @escaping Success, failure: @escaping Failure)
  func bannerIsOutOfScreen()
  func bannerIsOnScreen()

  var isNeedToRetain: Bool { get }
  var isPossibleToReload: Bool { get }
  var adID: String { get }
  var isBannerOnScreen: Bool { get }
}

// MARK: BannersCache
@objc (MWMBannersCache)
final class BannersCache: NSObject {

  static let cache = BannersCache()
  private override init() {}

  private var cache: [String : Cacheable] = [:]
  private var requests: Set<String> = []

  typealias Completion = (Any, Bool) -> Void
  private var completion: Completion?

  func get(_ key: String, completion comp: @escaping Completion) {
    completion = comp


    func onBannerFinished(_ cacheable: Cacheable, isAsync: Bool) {
      if let compl = completion {
        compl(cacheable, isAsync)
        cacheable.bannerIsOnScreen()
      }
    }

    if let cached = cache[key] {
      onBannerFinished(cached, isAsync: false)
      completion = nil;

      if !cached.isPossibleToReload || cached.isNeedToRetain {
        return
      }
    }

    if requests.contains(key) {
      return
    }

    let banner = CacheableFacebookBanner(placementID: key)

    banner.reload({ [weak self] cacheable in
      guard let s = self else { return }
      Statistics.logEvent(kStatPlacePageBannerShow,
                          withParameters: [kStatBanner : key, kStatProvider : kStatFacebook])

      s.add(cacheable)
      s.removeRequest(atKey: key)
      onBannerFinished(cacheable, isAsync: true)
    }, failure: { [weak self] event, errorDetails, error in
      guard let s = self else { return }
      Statistics.logEvent(event, withParameters: errorDetails)
      Crashlytics.sharedInstance().recordError(error)

      s.removeRequest(atKey: key)
    })

    addRequest(key)
  }

  func bannerIsOutOfScreen(_ bannerID: String) {
    if let cached = cache[bannerID], cached.isBannerOnScreen {
      cached.bannerIsOutOfScreen()
    }
  }

  private func add(_ banner: Cacheable) {
    cache[banner.adID] = banner
  }

  private func removeRequest(atKey key: String) {
    requests.remove(key)
  }

  private func addRequest(_ key: String) {
    requests.insert(key)
  }
}
