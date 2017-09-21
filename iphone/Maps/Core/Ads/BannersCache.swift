import Crashlytics
import MyTrackerSDK

@objc(MWMBannersCache)
final class BannersCache: NSObject {
  @objc static let cache = BannersCache()
  private override init() {}

  private enum LoadState {
    case notLoaded(BannerType)
    case loaded(BannerType)
    case error
  }

  typealias Completion = (MWMBanner, Bool) -> Void

  private var cache: [BannerType: Banner] = [:]
  private var requests: [BannerType: Banner] = [:]
  private var completion: Completion?
  private var loadStates: [LoadState]!
  private var cacheOnly = false

  private func onCompletion(isAsync: Bool) {
    guard let completion = completion else { return }
    var banner: Banner?
    statesLoop: for loadState in loadStates {
      switch loadState {
      case let .notLoaded(type):
        banner = cache[type]
        break statesLoop
      case let .loaded(type):
        banner = cache[type]
        break statesLoop
      case .error: continue
      }
    }
    if let banner = banner {
      Statistics.logEvent(kStatPlacePageBannerShow, withParameters: banner.statisticsDescription)
      MRMyTracker.trackEvent(withName: kStatPlacePageBannerShow)
      completion(banner, isAsync)
      banner.isBannerOnScreen = true
      self.completion = nil
    }
  }

  @objc func get(coreBanners: [CoreBanner], cacheOnly: Bool, loadNew: Bool = true, completion: @escaping Completion) {
    self.completion = completion
    self.cacheOnly = cacheOnly
    loadStates = coreBanners.map { coreBanner in
      let bannerType = BannerType(type: coreBanner.mwmType, id: coreBanner.bannerID)
      if let banner = cache[bannerType], (!banner.isPossibleToReload || banner.isNeedToRetain) {
        return .loaded(bannerType)
      } else {
        if loadNew {
          get(bannerType: bannerType)
        }
        return .notLoaded(bannerType)
      }
    }

    onCompletion(isAsync: false)
  }

  private func get(bannerType: BannerType) {
    guard requests[bannerType] == nil else { return }

    let banner = bannerType.banner!
    requests[bannerType] = banner
    banner.reload(success: { [unowned self] banner in
      self.setLoaded(banner: banner)
    }, failure: { [unowned self] bannerType, event, errorDetails, error in
      var statParams = errorDetails
      statParams[kStatErrorMessage] = (error as NSError).userInfo.reduce("") { $0 + "\($1.key) : \($1.value)\n" }
      Statistics.logEvent(event, withParameters: statParams)
      Crashlytics.sharedInstance().recordError(error)
      MRMyTracker.trackEvent(withName: event)
      self.setError(bannerType: bannerType)
    }, click: { banner in
      Statistics.logEvent(kStatPlacePageBannerClick, withParameters: banner.statisticsDescription)
      MRMyTracker.trackEvent(withName: kStatPlacePageBannerClick)
    })
  }

  private func notLoadedIndex(bannerType: BannerType) -> Array<LoadState>.Index? {
    return loadStates.index(where: {
      if case let .notLoaded(type) = $0, type == bannerType {
        return true
      }
      return false
    })
  }

  private func setLoaded(banner: Banner) {
    let bannerType = banner.type
    if let notLoadedIndex = notLoadedIndex(bannerType: bannerType) {
      loadStates[notLoadedIndex] = .loaded(bannerType)
    }
    cache[bannerType] = banner
    requests[bannerType] = nil
    if !cacheOnly {
      onCompletion(isAsync: true)
    }
  }

  private func setError(bannerType: BannerType) {
    if let notLoadedIndex = notLoadedIndex(bannerType: bannerType) {
      loadStates[notLoadedIndex] = .error
    }
    requests[bannerType] = nil
    onCompletion(isAsync: true)
  }

  @objc func bannerIsOutOfScreen(coreBanner: MWMBanner) {
    bannerIsOutOfScreen(bannerType: BannerType(type: coreBanner.mwmType, id: coreBanner.bannerID))
  }

  func bannerIsOutOfScreen(bannerType: BannerType) {
    completion = nil
    if let cached = cache[bannerType], cached.isBannerOnScreen {
      cached.isBannerOnScreen = false
    }
  }
}
