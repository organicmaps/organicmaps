@objc(MWMSearchBanners)
final class SearchBanners: NSObject {
  private var banners: [MWMBanner] = []

  weak var searchIndex: SearchIndex?

  @objc init(searchIndex: SearchIndex) {
    self.searchIndex = searchIndex
    super.init()
  }

  @objc func add(_ banner: MWMBanner) {
    guard let searchIndex = searchIndex else { return }
    banners.append(banner)
    let type: MWMSearchItemType
    let prefferedPosition: Int
    switch banner.mwmType {
    case .mopub:
      type = .mopub
      prefferedPosition = 2
    case .facebook:
      type = .facebook
      prefferedPosition = 2
    case .google:
      type = .google
      prefferedPosition = 4
    default:
      assert(false, "Unsupported banner type")
      type = .regular
      prefferedPosition = 0
    }
    searchIndex.addItem(type: type, prefferedPosition: prefferedPosition, containerIndex: banners.count - 1)
  }

  @objc func banner(atIndex index: Int) -> MWMBanner {
    return banners[index]
  }

  deinit {
    banners.forEach { BannersCache.cache.bannerIsOutOfScreen(coreBanner: $0) }
  }
}
