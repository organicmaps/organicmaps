import Crashlytics
import MyTrackerSDK

@objc(MWMBannersCache)
final class BannersCache: NSObject {
  @objc static let cache = BannersCache()
  private override init() {}

  private enum LoadState: Equatable {
    case notLoaded(BannerType)
    case loaded(BannerType)
    case error(BannerType)

    static func ==(lhs: LoadState, rhs: LoadState) -> Bool {
      switch (lhs, rhs) {
      case let (.notLoaded(l), .notLoaded(r)): return l == r
      case let (.loaded(l), .loaded(r)): return l == r
      case let (.error(l), .error(r)): return l == r
      case (.notLoaded, _),
           (.loaded, _),
           (.error, _): return false
      }
    }
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
      loadStates = nil
    }
  }

  @objc func get(coreBanners: [CoreBanner], cacheOnly: Bool, loadNew: Bool = true, completion: @escaping Completion) {
    self.completion = completion
    self.cacheOnly = cacheOnly
    loadStates = loadStates ?? []
    coreBanners.forEach { coreBanner in
      let bannerType = BannerType(type: coreBanner.mwmType, id: coreBanner.bannerID, query: coreBanner.query)
      if let banner = cache[bannerType], (!banner.isPossibleToReload || banner.isNeedToRetain) {
        appendLoadState(.loaded(bannerType))
      } else {
        if loadNew {
          get(bannerType: bannerType)
        }
        appendLoadState(.notLoaded(bannerType))
      }
    }

    onCompletion(isAsync: false)
  }

  @objc func refresh(coreBanners: [CoreBanner]) {
    loadStates = loadStates ?? []
    coreBanners.forEach { coreBanner in
      let bannerType = BannerType(type: coreBanner.mwmType, id: coreBanner.bannerID, query: coreBanner.query)
      let state = LoadState.notLoaded(bannerType)
      if loadStates.index(of: state) == nil {
        get(bannerType: bannerType)
        appendLoadState(state)
      }
    }
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

  private func appendLoadState(_ state: LoadState) {
    guard loadStates.index(of: state) == nil else { return }
    loadStates.append(state)
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
    cache[bannerType] = banner
    requests[bannerType] = nil

    guard loadStates != nil else { return }

    if let notLoadedIndex = loadStates.index(of: .notLoaded(bannerType)) {
      loadStates[notLoadedIndex] = .loaded(bannerType)
    }
    if !cacheOnly {
      onCompletion(isAsync: true)
    }
  }

  private func setError(bannerType: BannerType) {
    requests[bannerType] = nil

    guard loadStates != nil else { return }

    if let notLoadedIndex = loadStates.index(of: .notLoaded(bannerType)) {
      loadStates[notLoadedIndex] = .error(bannerType)
    }
    if !cacheOnly {
      onCompletion(isAsync: true)
    }
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
